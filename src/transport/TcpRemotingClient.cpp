/*
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "TcpRemotingClient.h"

#include <stddef.h>

#if !defined(WIN32) && !defined(__APPLE__)
#include <sys/prctl.h>
#endif

#include "Logging.h"
#include "MemoryOutputStream.h"
#include "UtilAll.h"

namespace rocketmq {

TcpRemotingClient::TcpRemotingClient(int pullThreadNum, uint64_t tcpConnectTimeout, uint64_t tcpTransportTryLockTimeout)
    : m_tcpConnectTimeout(tcpConnectTimeout),
      m_tcpTransportTryLockTimeout(tcpTransportTryLockTimeout),
      m_namesrvIndex(0),
      m_dispatchExecutor(1, false),
      m_handleExecutor(pullThreadNum, false),
      m_timeoutExecutor(false) {
#if !defined(WIN32) && !defined(__APPLE__)
  std::string taskName = UtilAll::getProcessName();
  prctl(PR_SET_NAME, "MessageDispatchExecutor", 0, 0, 0);
#endif
  m_dispatchExecutor.startup();
#if !defined(WIN32) && !defined(__APPLE__)
  prctl(PR_SET_NAME, taskName.c_str(), 0, 0, 0);
#endif

#if !defined(WIN32) && !defined(__APPLE__)
  prctl(PR_SET_NAME, "MessageHandleExecutor", 0, 0, 0);
#endif
  m_handleExecutor.startup();
#if !defined(WIN32) && !defined(__APPLE__)
  prctl(PR_SET_NAME, taskName.c_str(), 0, 0, 0);
#endif

  LOG_INFO("tcpConnectTimeout:%ju, tcpTransportTryLockTimeout:%ju, pullThreadNum:%d", m_tcpConnectTimeout,
           m_tcpTransportTryLockTimeout, pullThreadNum);

#if !defined(WIN32) && !defined(__APPLE__)
  prctl(PR_SET_NAME, "TimeoutScanExecutor", 0, 0, 0);
#endif
  m_timeoutExecutor.startup();
#if !defined(WIN32) && !defined(__APPLE__)
  prctl(PR_SET_NAME, taskName.c_str(), 0, 0, 0);
#endif
}

TcpRemotingClient::~TcpRemotingClient() {
  m_transportTable.clear();
  m_futureTable.clear();
  m_namesrvAddrList.clear();
}

void TcpRemotingClient::stopAllTcpTransportThread() {
  LOG_DEBUG("TcpRemotingClient::stopAllTcpTransportThread Begin");

  m_timeoutExecutor.shutdown();

  {
    std::lock_guard<std::timed_mutex> lock(m_transportTableMutex);
    for (const auto& trans : m_transportTable) {
      trans.second->disconnect(trans.first);
    }
    m_transportTable.clear();
  }

  m_handleExecutor.shutdown();

  m_dispatchExecutor.shutdown();

  {
    std::lock_guard<std::mutex> lock(m_futureTableMutex);
    for (const auto& future : m_futureTable) {
      if (future.second) {
        if (future.second->getInvokeCallback() != nullptr) {
          future.second->releaseThreadCondition();
        }
      }
    }
  }

  LOG_ERROR("TcpRemotingClient::stopAllTcpTransportThread End, m_transportTable:%lu", m_transportTable.size());
}

void TcpRemotingClient::registerRPCHook(std::shared_ptr<RPCHook> rpcHook) {
  if (rpcHook != nullptr) {
    for (auto& hook : m_rpcHooks) {
      if (hook == rpcHook) {
        return;
      }
    }

    m_rpcHooks.push_back(rpcHook);
  }
}

void TcpRemotingClient::updateNameServerAddressList(const std::string& addrs) {
  LOG_INFO("updateNameServerAddressList: [%s]", addrs.c_str());

  if (addrs.empty()) {
    return;
  }

  std::unique_lock<std::timed_mutex> lock(m_namesrvLock, std::try_to_lock);
  if (!lock.owns_lock()) {
    if (!lock.try_lock_for(std::chrono::seconds(10))) {
      LOG_ERROR("updateNameServerAddressList get timed_mutex timeout");
      return;
    }
  }

  // clear first;
  m_namesrvAddrList.clear();

  std::vector<std::string> out;
  UtilAll::Split(out, addrs, ";");
  for (auto& addr : out) {
    UtilAll::Trim(addr);

    std::string hostName;
    short portNumber;
    if (UtilAll::SplitURL(addr, hostName, portNumber)) {
      LOG_INFO("update Namesrv:%s", addr.c_str());
      m_namesrvAddrList.push_back(addr);
    } else {
      LOG_INFO("This may be invalid namer server: [%s]", addr.c_str());
    }
  }
  out.clear();
}

bool TcpRemotingClient::invokeHeartBeat(const std::string& addr, RemotingCommand& request, int timeoutMillis) {
  std::shared_ptr<TcpTransport> pTcp = GetTransport(addr, true);
  if (pTcp != nullptr) {
    int code = request.getCode();
    int opaque = request.getOpaque();

    std::shared_ptr<ResponseFuture> responseFuture(new ResponseFuture(code, opaque, timeoutMillis));
    addResponseFuture(opaque, responseFuture);

    if (SendCommand(pTcp, request)) {
      responseFuture->setSendRequestOK(true);
      std::unique_ptr<RemotingCommand> pRsp(responseFuture->waitResponse());
      if (pRsp == nullptr) {
        LOG_ERROR("wait response timeout of heartbeat, so closeTransport of addr:%s", addr.c_str());
        // avoid responseFuture leak;
        findAndDeleteResponseFuture(opaque);
        CloseTransport(addr, pTcp);
        return false;
      } else if (pRsp->getCode() == SUCCESS_VALUE) {
        return true;
      } else {
        LOG_WARN("get error response:%d of heartbeat to addr:%s", pRsp->getCode(), addr.c_str());
        return false;
      }
    } else {
      // avoid responseFuture leak;
      findAndDeleteResponseFuture(opaque);
      CloseTransport(addr, pTcp);
    }
  }
  return false;
}

RemotingCommand* TcpRemotingClient::invokeSync(const std::string& addr, RemotingCommand& request, int timeoutMillis) {
  LOG_DEBUG("InvokeSync:", addr.c_str());
  std::shared_ptr<TcpTransport> pTcp = GetTransport(addr, true);
  if (pTcp != nullptr) {
    try {
      doBeforeRpcHooks(addr, request, true);
      RemotingCommand* response = invokeSyncImpl(pTcp, request, timeoutMillis);
      doAfterRpcHooks(addr, request, response, false);
      return response;
    } catch (const RemotingTimeoutException& e) {
      LOG_WARN("invokeSync: wait response timeout exception, the channel[{}]", pTcp->getPeerAddrAndPort().c_str());
      int code = request.getCode();
      if (code != GET_CONSUMER_LIST_BY_GROUP) {
        LOG_WARN("invokeSync: close socket because of timeout, {}ms, {}", timeoutMillis,
                 pTcp->getPeerAddrAndPort().c_str());
        CloseTransport(addr, pTcp);
      }
    } catch (const RemotingSendRequestException& e) {
      LOG_WARN("invokeSync: send request exception, so close the channel[%s]", pTcp->getPeerAddrAndPort().c_str());
      CloseTransport(addr, pTcp);
    }
  } else {
    LOG_DEBUG("InvokeSync [%s] Failed: Cannot Get Transport.", addr.c_str());
  }
  return nullptr;
}

RemotingCommand* TcpRemotingClient::invokeSyncImpl(std::shared_ptr<TcpTransport> pTcp,
                                                   RemotingCommand& request,
                                                   int64 timeoutMillis) throw(RemotingTimeoutException,
                                                                              RemotingSendRequestException) {
  int code = request.getCode();
  int opaque = request.getOpaque();

  std::shared_ptr<ResponseFuture> responseFuture(new ResponseFuture(code, opaque, timeoutMillis));
  addResponseFuture(opaque, responseFuture);

  if (SendCommand(pTcp, request)) {
    responseFuture->setSendRequestOK(true);
    RemotingCommand* response = responseFuture->waitResponse();
    if (response != nullptr) {
      return response;
    }
  }

  // avoid responseFuture leak;
  findAndDeleteResponseFuture(opaque);

  if (responseFuture->isSendRequestOK()) {
    THROW_MQEXCEPTION(RemotingTimeoutException,
                      "wait response on the addr <" + pTcp->getPeerAddrAndPort() + "> timeout", -1);
  } else {
    THROW_MQEXCEPTION(RemotingSendRequestException, "send request to <" + pTcp->getPeerAddrAndPort() + "> failed", -1);
  }
}

bool TcpRemotingClient::invokeAsync(const std::string& addr,
                                    RemotingCommand& request,
                                    InvokeCallback* invokeCallback,
                                    int64 timeoutMillis) {
  std::shared_ptr<TcpTransport> pTcp = GetTransport(addr, true);
  if (pTcp != nullptr) {
    try {
      doBeforeRpcHooks(addr, request, true);
      invokeAsyncImpl(pTcp, request, timeoutMillis, invokeCallback);
      return true;
    } catch (const RemotingSendRequestException& e) {
      LOG_WARN("invokeAsync: send request exception, so close the channel[%s]", pTcp->getPeerAddrAndPort().c_str());
      CloseTransport(addr, pTcp);
    }
  }

  LOG_ERROR("invokeAsync failed of addr:%s", addr.c_str());
  return false;
}

void TcpRemotingClient::invokeAsyncImpl(std::shared_ptr<TcpTransport> pTcp,
                                        RemotingCommand& request,
                                        int64 timeoutMillis,
                                        InvokeCallback* invokeCallback) throw(RemotingSendRequestException) {
  int code = request.getCode();
  int opaque = request.getOpaque();

  // delete in callback
  std::shared_ptr<ResponseFuture> responseFuture(new ResponseFuture(code, opaque, timeoutMillis, invokeCallback));
  addResponseFuture(opaque, responseFuture);

  // timeout monitor
  m_timeoutExecutor.schedule(std::bind(&TcpRemotingClient::checkAsyncRequestTimeout, this, opaque), timeoutMillis,
                             time_unit::milliseconds);

  if (SendCommand(pTcp, request)) {
    responseFuture->setSendRequestOK(true);
  } else {
    THROW_MQEXCEPTION(RemotingSendRequestException, "send request to <" + pTcp->getPeerAddrAndPort() + "> failed", -1);
  }
}

void TcpRemotingClient::invokeOneway(const std::string& addr, RemotingCommand& request) {
  // not need callback;
  std::shared_ptr<TcpTransport> pTcp = GetTransport(addr, true);
  if (pTcp != nullptr) {
    try {
      doBeforeRpcHooks(addr, request, true);
      invokeOnewayImpl(pTcp, request);
    } catch (const RemotingSendRequestException& e) {
      LOG_WARN("invokeOneway: send request exception, so close the channel[%s]", pTcp->getPeerAddrAndPort().c_str());
      CloseTransport(addr, pTcp);
    }
  } else {
    LOG_WARN("invokeOneway failed: NULL transport. addr:%s, code:%d", addr.c_str(), request.getCode());
  }
}

void TcpRemotingClient::invokeOnewayImpl(std::shared_ptr<TcpTransport> pTcp, RemotingCommand& request) {
  request.markOnewayRPC();
  if (!SendCommand(pTcp, request)) {
    THROW_MQEXCEPTION(RemotingSendRequestException, "send request to <" + pTcp->getPeerAddrAndPort() + "> failed", -1);
  }
}

void TcpRemotingClient::doBeforeRpcHooks(const std::string& addr, RemotingCommand& request, bool toSent) {
  if (m_rpcHooks.size() > 0) {
    for (auto& rpcHook : m_rpcHooks) {
      rpcHook->doBeforeRequest(addr, request, toSent);
    }
  }
}

void TcpRemotingClient::doAfterRpcHooks(const std::string& addr,
                                        RemotingCommand& request,
                                        RemotingCommand* response,
                                        bool toSent) {
  if (m_rpcHooks.size() > 0) {
    for (auto& rpcHook : m_rpcHooks) {
      rpcHook->doAfterResponse(addr, request, response, toSent);
    }
  }
}

std::shared_ptr<TcpTransport> TcpRemotingClient::GetTransport(const std::string& addr, bool needResponse) {
  if (addr.empty()) {
    LOG_DEBUG("GetTransport of NameServer");
    return CreateNameServerTransport(needResponse);
  }
  return CreateTransport(addr, needResponse);
}

std::shared_ptr<TcpTransport> TcpRemotingClient::CreateTransport(const std::string& addr, bool needResponse) {
  std::shared_ptr<TcpTransport> tts;

  {
    // try get m_tcpLock util m_tcpTransportTryLockTimeout to avoid blocking
    // long time, if could not get m_tcpLock, return NULL
    std::unique_lock<std::timed_mutex> lock(m_transportTableMutex, std::try_to_lock);
    if (!lock.owns_lock()) {
      if (!lock.try_lock_for(std::chrono::seconds(m_tcpTransportTryLockTimeout))) {
        LOG_ERROR("GetTransport of:%s get timed_mutex timeout", addr.c_str());
        std::shared_ptr<TcpTransport> pTcp;
        return pTcp;
      }
    }

    // check for reuse
    if (m_transportTable.find(addr) != m_transportTable.end()) {
      std::shared_ptr<TcpTransport> tcp = m_transportTable[addr];

      if (tcp) {
        TcpConnectStatus connectStatus = tcp->getTcpConnectStatus();
        if (connectStatus == TCP_CONNECT_STATUS_SUCCESS) {
          return tcp;
        } else if (connectStatus == TCP_CONNECT_STATUS_WAIT) {
          std::shared_ptr<TcpTransport> pTcp;
          return pTcp;
        } else if (connectStatus == TCP_CONNECT_STATUS_FAILED) {
          LOG_ERROR("tcpTransport with server disconnected, erase server:%s", addr.c_str());
          tcp->disconnect(addr);  // avoid coredump when connection with broker was broken
          m_transportTable.erase(addr);
        } else {
          LOG_ERROR("go to fault state, erase:%s from tcpMap, and reconnect it", addr.c_str());
          m_transportTable.erase(addr);
        }
      }
    }

    // callback;
    TcpTransportReadCallback callback = needResponse ? &TcpRemotingClient::MessageReceived : nullptr;

    tts = TcpTransport::CreateTransport(this, callback);
    TcpConnectStatus connectStatus = tts->connect(addr, 0);  // use non-block
    if (connectStatus != TCP_CONNECT_STATUS_WAIT) {
      LOG_WARN("can not connect to:%s", addr.c_str());
      tts->disconnect(addr);
      std::shared_ptr<TcpTransport> pTcp;
      return pTcp;
    } else {
      // even if connecting failed finally, this server transport will be erased by next CreateTransport
      m_transportTable[addr] = tts;
    }
  }

  TcpConnectStatus connectStatus = tts->waitTcpConnectEvent(static_cast<int>(m_tcpConnectTimeout));
  if (connectStatus != TCP_CONNECT_STATUS_SUCCESS) {
    LOG_WARN("can not connect to server:%s", addr.c_str());
    tts->disconnect(addr);
    std::shared_ptr<TcpTransport> pTcp;
    return pTcp;
  } else {
    LOG_INFO("connect server with addr:%s success", addr.c_str());
    return tts;
  }
}

std::shared_ptr<TcpTransport> TcpRemotingClient::CreateNameServerTransport(bool needResponse) {
  // m_namesrvLock was added to avoid operation of nameServer was blocked by
  // m_tcpLock, it was used by single Thread mostly, so no performance impact
  // try get m_tcpLock until m_tcpTransportTryLockTimeout to avoid blocking long
  // time, if could not get m_namesrvlock, return NULL
  LOG_DEBUG("--CreateNameserverTransport--");
  std::unique_lock<std::timed_mutex> lock(m_namesrvLock, std::try_to_lock);
  if (!lock.owns_lock()) {
    if (!lock.try_lock_for(std::chrono::seconds(m_tcpTransportTryLockTimeout))) {
      LOG_ERROR("CreateNameserverTransport get timed_mutex timeout");
      std::shared_ptr<TcpTransport> pTcp;
      return pTcp;
    }
  }

  if (!m_namesrvAddrChoosed.empty()) {
    std::shared_ptr<TcpTransport> pTcp = CreateTransport(m_namesrvAddrChoosed, true);
    if (pTcp)
      return pTcp;
    else
      m_namesrvAddrChoosed.clear();
  }

  for (unsigned i = 0; i < m_namesrvAddrList.size(); i++) {
    unsigned int index = m_namesrvIndex++ % m_namesrvAddrList.size();
    LOG_INFO("namesrvIndex is:%d, index:%d, namesrvaddrlist size:" SIZET_FMT "", m_namesrvIndex, index,
             m_namesrvAddrList.size());
    std::shared_ptr<TcpTransport> pTcp = CreateTransport(m_namesrvAddrList[index], true);
    if (pTcp) {
      m_namesrvAddrChoosed = m_namesrvAddrList[index];
      return pTcp;
    }
  }

  std::shared_ptr<TcpTransport> pTcp;
  return pTcp;
}

bool TcpRemotingClient::CloseTransport(const std::string& addr, std::shared_ptr<TcpTransport> pTcp) {
  if (addr.empty()) {
    return CloseNameServerTransport(pTcp);
  }

  std::unique_lock<std::timed_mutex> lock(m_transportTableMutex, std::try_to_lock);
  if (!lock.owns_lock()) {
    if (!lock.try_lock_for(std::chrono::seconds(m_tcpTransportTryLockTimeout))) {
      LOG_ERROR("CloseTransport of:%s get timed_mutex timeout", addr.c_str());
      return false;
    }
  }

  LOG_ERROR("CloseTransport of:%s", addr.c_str());

  bool removeItemFromTable = true;
  if (m_transportTable.find(addr) != m_transportTable.end()) {
    if (m_transportTable[addr]->getStartTime() != pTcp->getStartTime()) {
      LOG_INFO("tcpTransport with addr:%s has been closed before, and has been created again, nothing to do",
               addr.c_str());
      removeItemFromTable = false;
    }
  } else {
    LOG_INFO("tcpTransport with addr:%s had been removed from tcpTable before", addr.c_str());
    removeItemFromTable = false;
  }

  if (removeItemFromTable) {
    LOG_WARN("closeTransport: disconnect:%s with state:%d", addr.c_str(),
             m_transportTable[addr]->getTcpConnectStatus());
    if (m_transportTable[addr]->getTcpConnectStatus() == TCP_CONNECT_STATUS_SUCCESS)
      m_transportTable[addr]->disconnect(addr);  // avoid coredump when connection with server was broken
    LOG_WARN("closeTransport: erase broker: %s", addr.c_str());
    m_transportTable.erase(addr);
  }

  LOG_ERROR("CloseTransport of:%s end", addr.c_str());

  return removeItemFromTable;
}

bool TcpRemotingClient::CloseNameServerTransport(std::shared_ptr<TcpTransport> pTcp) {
  std::unique_lock<std::timed_mutex> lock(m_namesrvLock, std::try_to_lock);
  if (!lock.owns_lock()) {
    if (!lock.try_lock_for(std::chrono::seconds(m_tcpTransportTryLockTimeout))) {
      LOG_ERROR("CreateNameServerTransport get timed_mutex timeout");
      return false;
    }
  }

  std::string addr = m_namesrvAddrChoosed;

  bool removeItemFromTable = CloseTransport(addr, pTcp);
  if (removeItemFromTable) {
    m_namesrvAddrChoosed.clear();
  }

  return removeItemFromTable;
}

bool TcpRemotingClient::SendCommand(std::shared_ptr<TcpTransport> pTts, RemotingCommand& msg) {
  const MemoryBlock* pHead = msg.GetHead();
  const MemoryBlock* pBody = msg.GetBody();

  std::unique_ptr<MemoryOutputStream> buffer(new MemoryOutputStream(1024));
  if (pHead->getSize() > 0) {
    buffer->write(pHead->getData(), static_cast<size_t>(pHead->getSize()));
  }
  if (pBody->getSize() > 0) {
    buffer->write(pBody->getData(), static_cast<size_t>(pBody->getSize()));
  }

  const char* pData = static_cast<const char*>(buffer->getData());
  size_t len = buffer->getDataSize();
  return pTts->sendMessage(pData, len);
}

void TcpRemotingClient::MessageReceived(void* context, const MemoryBlock& mem, const std::string& addr) {
  auto* pTcpRemotingClient = reinterpret_cast<TcpRemotingClient*>(context);
  if (pTcpRemotingClient)
    pTcpRemotingClient->messageReceived(mem, addr);
}

void TcpRemotingClient::messageReceived(const MemoryBlock& mem, const std::string& addr) {
  m_dispatchExecutor.submit(std::bind(&TcpRemotingClient::processMessageReceived, this, mem, addr));
}

void TcpRemotingClient::processMessageReceived(const MemoryBlock& mem, const std::string& addr) {
  RemotingCommand* cmd = nullptr;
  try {
    cmd = RemotingCommand::Decode(mem);
  } catch (...) {
    LOG_ERROR("processMessageReceived error");
    return;
  }

  if (cmd->isResponseType()) {
    processResponseCommand(cmd);
  } else {
    m_handleExecutor.submit(std::bind(&TcpRemotingClient::processRequestCommand, this, cmd, addr));
  }
}

void TcpRemotingClient::processResponseCommand(RemotingCommand* cmd) {
  int opaque = cmd->getOpaque();
  std::shared_ptr<ResponseFuture> responseFuture = findAndDeleteResponseFuture(opaque);
  if (responseFuture) {
    int code = responseFuture->getRequestCode();
    cmd->SetExtHeader(code);  // decode CommandHeader

    LOG_DEBUG("processResponseCommand, opaque:%d, code:%d");

    if (responseFuture->getInvokeCallback() != nullptr) {
      responseFuture->setResponseCommand(cmd);
      // bind shared_ptr can save object's life
      m_handleExecutor.submit(std::bind(&ResponseFuture::executeInvokeCallback, responseFuture));
    } else {
      responseFuture->putResponse(cmd);
    }
  } else {
    LOG_DEBUG("responseFuture was deleted by timeout of opaque:%d", opaque);
    deleteAndZero(cmd);
  }
}

void TcpRemotingClient::checkAsyncRequestTimeout(int opaque) {
  LOG_DEBUG("checkAsyncRequestTimeout opaque:%d", opaque);

  std::shared_ptr<ResponseFuture> responseFuture(findAndDeleteResponseFuture(opaque));
  if (responseFuture) {
    LOG_ERROR("no response got for opaque:%d", opaque);
    if (responseFuture->getInvokeCallback() != nullptr) {
      m_handleExecutor.submit(std::bind(&ResponseFuture::executeInvokeCallback, responseFuture));
    }
  }
}

void TcpRemotingClient::processRequestCommand(RemotingCommand* cmd, const std::string& addr) {
  std::unique_ptr<RemotingCommand> requestCommand(cmd);
  int requestCode = requestCommand->getCode();
  if (m_processorTable.find(requestCode) == m_processorTable.end()) {
    // TODO: send REQUEST_CODE_NOT_SUPPORTED response
    LOG_ERROR("can_not_find request:%d processor", requestCode);
  } else {
    try {
      auto& processor = iter->second;

      doBeforeRpcHooks(addr, *requestCommand, false);
      std::unique_ptr<RemotingCommand> response(processor->processRequest(addr, requestCommand.get()));
      doAfterRpcHooks(addr, *requestCommand, response.get(), true);

      if (!requestCommand->isOnewayRPC()) {
        if (response) {
          response->setOpaque(requestCommand->getOpaque());
          response->markResponseType();
          response->Encode();
          invokeOneway(addr, *response);
        }
      }
    } catch (...) {
      if (!cmd->isOnewayRPC()) {
        // TODO: send SYSTEM_ERROR response
      }
    }
  }
}

void TcpRemotingClient::addResponseFuture(int opaque, std::shared_ptr<ResponseFuture> pFuture) {
  std::lock_guard<std::mutex> lock(m_futureTableMutex);
  m_futureTable[opaque] = pFuture;
}

// Note: after call this function, shared_ptr of m_futureTable[opaque] will
// be erased, so caller must ensure the life cycle of returned shared_ptr;
std::shared_ptr<ResponseFuture> TcpRemotingClient::findAndDeleteResponseFuture(int opaque) {
  std::lock_guard<std::mutex> lock(m_futureTableMutex);
  std::shared_ptr<ResponseFuture> pResponseFuture;
  if (m_futureTable.find(opaque) != m_futureTable.end()) {
    pResponseFuture = m_futureTable[opaque];
    m_futureTable.erase(opaque);
  }
  return pResponseFuture;
}

void TcpRemotingClient::registerProcessor(MQRequestCode requestCode, ClientRemotingProcessor* clientRemotingProcessor) {
  if (m_processorTable.find(requestCode) != m_processorTable.end())
    m_processorTable.erase(requestCode);
  m_processorTable[requestCode] = clientRemotingProcessor;
}

}  // namespace rocketmq
