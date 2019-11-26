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

#include "Logging.h"
#include "MemoryOutputStream.h"
#include "UtilAll.h"

namespace rocketmq {

TcpRemotingClient::TcpRemotingClient(int workerThreadNum,
                                     uint64_t tcpConnectTimeout,
                                     uint64_t tcpTransportTryLockTimeout)
    : m_tcpConnectTimeout(tcpConnectTimeout),
      m_tcpTransportTryLockTimeout(tcpTransportTryLockTimeout),
      m_namesrvIndex(0),
      m_dispatchExecutor("MessageDispatchExecutor", 1, false),
      m_handleExecutor("MessageHandleExecutor", workerThreadNum, false),
      m_timeoutExecutor("TimeoutScanExecutor", false) {}

TcpRemotingClient::~TcpRemotingClient() = default;

void TcpRemotingClient::start() {
  m_dispatchExecutor.startup();
  m_handleExecutor.startup();

  LOG_INFO("tcpConnectTimeout:%ju, tcpTransportTryLockTimeout:%ju, pullThreadNum:%d", m_tcpConnectTimeout,
           m_tcpTransportTryLockTimeout, m_handleExecutor.num_threads());

  m_timeoutExecutor.startup();

  // scanResponseTable
  m_timeoutExecutor.schedule(std::bind(&TcpRemotingClient::scanResponseTablePeriodically, this), 1000 * 3,
                             time_unit::milliseconds);
}

void TcpRemotingClient::shutdown() {
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

  if (!UtilAll::try_lock_for(m_namesrvLock, 1000 * 10)) {
    LOG_ERROR("updateNameServerAddressList get timed_mutex timeout");
    return;
  }
  std::lock_guard<std::timed_mutex> lock(m_namesrvLock, std::adopt_lock);

  // clear first
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
}

void TcpRemotingClient::scanResponseTablePeriodically() {
  try {
    scanResponseTable();
  } catch (std::exception& e) {
    LOG_ERROR("scanResponseTable exception: %s", e.what());
  }

  // next round
  m_timeoutExecutor.schedule(std::bind(&TcpRemotingClient::scanResponseTablePeriodically, this), 1000,
                             time_unit::milliseconds);
}

void TcpRemotingClient::scanResponseTable() {
  std::vector<std::shared_ptr<ResponseFuture>> rfList;

  {
    std::lock_guard<std::mutex> lock(m_futureTableMutex);
    auto now = UtilAll::currentTimeMillis();
    for (auto it = m_futureTable.begin(); it != m_futureTable.end();) {
      auto& rep = it->second;  // NOTE: rep is a reference
      if (rep->getBeginTimestamp() + rep->getTimeoutMillis() + 1000 <= now) {
        LOG_WARN_NEW("remove timeout request, code:{}, opaque:{}", rep->getRequestCode(), rep->getOpaque());
        rfList.push_back(rep);
        it = m_futureTable.erase(it);
      } else {
        ++it;
      }
    }
  }

  for (auto rf : rfList) {
    if (rf->getInvokeCallback() != nullptr) {
      m_handleExecutor.submit(std::bind(&ResponseFuture::executeInvokeCallback, rf));
    }
  }
}

RemotingCommand* TcpRemotingClient::invokeSync(const std::string& addr,
                                               RemotingCommand& request,
                                               int timeoutMillis) throw(RemotingException) {
  auto beginStartTime = UtilAll::currentTimeMillis();
  TcpTransportPtr channel = GetTransport(addr, true);
  if (channel != nullptr) {
    try {
      doBeforeRpcHooks(addr, request, true);
      auto costTime = UtilAll::currentTimeMillis() - beginStartTime;
      if (timeoutMillis <= 0 || timeoutMillis < costTime) {
        THROW_MQEXCEPTION(RemotingTimeoutException, "invokeSync call timeout", -1);
      }
      RemotingCommand* response = invokeSyncImpl(channel, request, timeoutMillis);
      doAfterRpcHooks(addr, request, response, false);
      return response;
    } catch (const RemotingSendRequestException& e) {
      LOG_WARN_NEW("invokeSync: send request exception, so close the channel[{}]", channel->getPeerAddrAndPort());
      CloseTransport(addr, channel);
      throw e;
    } catch (const RemotingTimeoutException& e) {
      int code = request.getCode();
      if (code != GET_CONSUMER_LIST_BY_GROUP) {
        CloseTransport(addr, channel);
        LOG_WARN_NEW("invokeSync: close socket because of timeout, {}ms, {}", timeoutMillis,
                     channel->getPeerAddrAndPort());
      }
      LOG_WARN_NEW("invokeSync: wait response timeout exception, the channel[{}]", channel->getPeerAddrAndPort());
      throw e;
    }
  } else {
    THROW_MQEXCEPTION(RemotingConnectException, "connect to <" + addr + "> failed", -1);
  }
}

RemotingCommand* TcpRemotingClient::invokeSyncImpl(TcpTransportPtr channel,
                                                   RemotingCommand& request,
                                                   int64_t timeoutMillis) throw(RemotingTimeoutException,
                                                                                RemotingSendRequestException) {
  int code = request.getCode();
  int opaque = request.getOpaque();

  std::shared_ptr<ResponseFuture> responseFuture(new ResponseFuture(code, opaque, timeoutMillis));
  addResponseFuture(opaque, responseFuture);

  if (SendCommand(channel, request)) {
    responseFuture->setSendRequestOK(true);
  } else {
    responseFuture->setSendRequestOK(false);

    findAndDeleteResponseFuture(opaque);
    responseFuture->putResponse(nullptr);

    LOG_WARN_NEW("send a request command to channel <{}}> failed.", channel->getPeerAddrAndPort());
  }

  RemotingCommand* response = responseFuture->waitResponse();
  if (nullptr == response) {
    if (responseFuture->isSendRequestOK()) {
      THROW_MQEXCEPTION(RemotingTimeoutException,
                        "wait response on the addr <" + channel->getPeerAddrAndPort() + "> timeout", -1);
    } else {
      THROW_MQEXCEPTION(RemotingSendRequestException, "send request to <" + channel->getPeerAddrAndPort() + "> failed",
                        -1);
    }
  }

  return response;
}

void TcpRemotingClient::invokeAsync(const std::string& addr,
                                    RemotingCommand& request,
                                    InvokeCallback* invokeCallback,
                                    int64_t timeoutMillis) throw(RemotingException) {
  auto beginStartTime = UtilAll::currentTimeMillis();
  TcpTransportPtr channel = GetTransport(addr, true);
  if (channel != nullptr) {
    try {
      doBeforeRpcHooks(addr, request, true);
      auto costTime = UtilAll::currentTimeMillis() - beginStartTime;
      if (timeoutMillis <= 0 || timeoutMillis < costTime) {
        THROW_MQEXCEPTION(RemotingTooMuchRequestException, "invokeAsync call timeout", -1);
      }
      invokeAsyncImpl(channel, request, timeoutMillis, invokeCallback);
    } catch (const RemotingSendRequestException& e) {
      LOG_WARN("invokeAsync: send request exception, so close the channel[%s]", channel->getPeerAddrAndPort().c_str());
      CloseTransport(addr, channel);
      throw e;
    }
  } else {
    THROW_MQEXCEPTION(RemotingConnectException, "connect to <" + addr + "> failed", -1);
  }
}

void TcpRemotingClient::invokeAsyncImpl(TcpTransportPtr channel,
                                        RemotingCommand& request,
                                        int64_t timeoutMillis,
                                        InvokeCallback* invokeCallback) throw(RemotingSendRequestException) {
  int code = request.getCode();
  int opaque = request.getOpaque();

  // delete in callback
  std::shared_ptr<ResponseFuture> responseFuture(new ResponseFuture(code, opaque, timeoutMillis, invokeCallback));
  addResponseFuture(opaque, responseFuture);

  try {
    if (SendCommand(channel, request)) {
      responseFuture->setSendRequestOK(true);
    } else {
      // requestFail
      responseFuture = findAndDeleteResponseFuture(opaque);
      if (responseFuture != nullptr) {
        responseFuture->setSendRequestOK(false);
        responseFuture->putResponse(nullptr);
        if (responseFuture->getInvokeCallback() != nullptr) {
          m_handleExecutor.submit(std::bind(&ResponseFuture::executeInvokeCallback, responseFuture));
        }
      }

      LOG_WARN_NEW("send a request command to channel <{}> failed.", channel->getPeerAddrAndPort());
    }
  } catch (const std::exception& e) {
    LOG_WARN_NEW("send a request command to channel <{}> Exception.\n{}", channel->getPeerAddrAndPort(), e.what());
    THROW_MQEXCEPTION(RemotingSendRequestException, "send request to <" + channel->getPeerAddrAndPort() + "> failed",
                      -1);
  }
}

void TcpRemotingClient::invokeOneway(const std::string& addr, RemotingCommand& request) throw(RemotingException) {
  TcpTransportPtr channel = GetTransport(addr, true);
  if (channel != nullptr) {
    try {
      doBeforeRpcHooks(addr, request, true);
      invokeOnewayImpl(channel, request);
    } catch (const RemotingSendRequestException& e) {
      LOG_WARN("invokeOneway: send request exception, so close the channel[{}]", channel->getPeerAddrAndPort());
      CloseTransport(addr, channel);
      throw e;
    }
  } else {
    THROW_MQEXCEPTION(RemotingConnectException, "connect to <" + addr + "> failed", -1);
  }
}

void TcpRemotingClient::invokeOnewayImpl(TcpTransportPtr channel,
                                         RemotingCommand& request) throw(RemotingSendRequestException) {
  request.markOnewayRPC();
  try {
    if (!SendCommand(channel, request)) {
      LOG_WARN_NEW("send a request command to channel <{}> failed.", channel->getPeerAddrAndPort());
    }
  } catch (const std::exception& e) {
    LOG_WARN_NEW("send a request command to channel <{}> Exception.\n{}", channel->getPeerAddrAndPort(), e.what());
    THROW_MQEXCEPTION(RemotingSendRequestException, "send request to <" + channel->getPeerAddrAndPort() + "> failed",
                      -1);
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

TcpTransportPtr TcpRemotingClient::GetTransport(const std::string& addr, bool needResponse) {
  if (addr.empty()) {
    LOG_DEBUG("GetTransport of NameServer");
    return CreateNameServerTransport(needResponse);
  }
  return CreateTransport(addr, needResponse);
}

TcpTransportPtr TcpRemotingClient::CreateTransport(const std::string& addr, bool needResponse) {
  TcpTransportPtr channel;

  {
    // try get m_tcpLock util m_tcpTransportTryLockTimeout to avoid blocking long time,
    // if could not get m_transportTableMutex, return NULL
    if (!UtilAll::try_lock_for(m_transportTableMutex, 1000 * m_tcpTransportTryLockTimeout)) {
      LOG_ERROR("GetTransport of:%s get timed_mutex timeout", addr.c_str());
      return TcpTransportPtr();
    }
    std::lock_guard<std::timed_mutex> lock(m_transportTableMutex, std::adopt_lock);

    // check for reuse
    auto iter = m_transportTable.find(addr);
    if (iter != m_transportTable.end()) {
      channel = iter->second;
      if (channel != nullptr) {
        TcpConnectStatus connectStatus = channel->getTcpConnectStatus();
        switch (connectStatus) {
          case TCP_CONNECT_STATUS_CONNECTED:
            return channel;
          case TCP_CONNECT_STATUS_CONNECTING:
            // wait server answer, return dummy
            return TcpTransportPtr();
          case TCP_CONNECT_STATUS_FAILED:
            LOG_ERROR_NEW("tcpTransport with server disconnected, erase server:{}", addr);
            channel->disconnect(addr);  // avoid coredump when connection with broker was broken
            m_transportTable.erase(addr);
            break;
          default:
            LOG_ERROR_NEW("go to CLOSED state, erase:{} from transportTable, and reconnect it", addr);
            m_transportTable.erase(addr);
            break;
        }
      }
    }

    // choose callback
    TcpTransportReadCallback callback = needResponse ? &TcpRemotingClient::MessageReceived : nullptr;

    // create new transport, then connect server
    channel = TcpTransport::CreateTransport(this, callback);
    TcpConnectStatus connectStatus = channel->connect(addr, 0);  // use non-block
    if (connectStatus != TCP_CONNECT_STATUS_CONNECTING) {
      LOG_WARN("can not connect to:{}", addr);
      channel->disconnect(addr);
      return TcpTransportPtr();
    } else {
      // even if connecting failed finally, this server transport will be erased by next CreateTransport
      m_transportTable[addr] = channel;
    }
  }

  // waiting...
  TcpConnectStatus connectStatus = channel->waitTcpConnectEvent(static_cast<int>(m_tcpConnectTimeout));
  if (connectStatus != TCP_CONNECT_STATUS_CONNECTED) {
    LOG_WARN("can not connect to server:%s", addr.c_str());
    channel->disconnect(addr);
    return TcpTransportPtr();
  } else {
    LOG_INFO("connect server with addr:%s success", addr.c_str());
    return channel;
  }
}

TcpTransportPtr TcpRemotingClient::CreateNameServerTransport(bool needResponse) {
  // m_namesrvLock was added to avoid operation of nameServer was blocked by
  // m_tcpLock, it was used by single Thread mostly, so no performance impact
  // try get m_tcpLock until m_tcpTransportTryLockTimeout to avoid blocking long
  // time, if could not get m_namesrvlock, return NULL
  LOG_DEBUG("--CreateNameserverTransport--");
  if (!UtilAll::try_lock_for(m_namesrvLock, 1000 * m_tcpTransportTryLockTimeout)) {
    LOG_ERROR("CreateNameserverTransport get timed_mutex timeout");
    return TcpTransportPtr();
  }
  std::lock_guard<std::timed_mutex> lock(m_namesrvLock, std::adopt_lock);

  if (!m_namesrvAddrChoosed.empty()) {
    TcpTransportPtr channel = CreateTransport(m_namesrvAddrChoosed, true);
    if (channel != nullptr) {
      return channel;
    } else {
      m_namesrvAddrChoosed.clear();
    }
  }

  for (unsigned i = 0; i < m_namesrvAddrList.size(); i++) {
    unsigned int index = m_namesrvIndex++ % m_namesrvAddrList.size();
    LOG_INFO("namesrvIndex is:%d, index:%d, namesrvaddrlist size:" SIZET_FMT "", m_namesrvIndex, index,
             m_namesrvAddrList.size());
    TcpTransportPtr channel = CreateTransport(m_namesrvAddrList[index], true);
    if (channel != nullptr) {
      m_namesrvAddrChoosed = m_namesrvAddrList[index];
      return channel;
    }
  }

  return TcpTransportPtr();
}

bool TcpRemotingClient::CloseTransport(const std::string& addr, TcpTransportPtr channel) {
  if (addr.empty()) {
    return CloseNameServerTransport(channel);
  }

  if (!UtilAll::try_lock_for(m_transportTableMutex, 1000 * m_tcpTransportTryLockTimeout)) {
    LOG_ERROR_NEW("CloseTransport of:{} get timed_mutex timeout", addr);
    return false;
  }
  std::lock_guard<std::timed_mutex> lock(m_transportTableMutex, std::adopt_lock);

  LOG_ERROR_NEW("CloseTransport of:{}", addr);

  bool removeItemFromTable = true;
  if (m_transportTable.find(addr) != m_transportTable.end()) {
    if (m_transportTable[addr]->getStartTime() != channel->getStartTime()) {
      LOG_INFO_NEW("tcpTransport with addr:{} has been closed before, and has been created again, nothing to do", addr);
      removeItemFromTable = false;
    }
  } else {
    LOG_INFO_NEW("tcpTransport with addr:{} had been removed from tcpTable before", addr);
    removeItemFromTable = false;
  }

  if (removeItemFromTable) {
    LOG_WARN_NEW("closeTransport: erase broker: {}", addr);
    m_transportTable.erase(addr);
  }

  LOG_WARN_NEW("closeTransport: disconnect:{} with state:{}", addr, channel->getTcpConnectStatus());
  if (channel->getTcpConnectStatus() != TCP_CONNECT_STATUS_CLOSED) {
    channel->disconnect(addr);  // avoid coredump when connection with server was broken
  }

  LOG_ERROR_NEW("CloseTransport of:{} end", addr);

  return removeItemFromTable;
}

bool TcpRemotingClient::CloseNameServerTransport(TcpTransportPtr channel) {
  if (!UtilAll::try_lock_for(m_namesrvLock, 1000 * m_tcpTransportTryLockTimeout)) {
    LOG_ERROR("CloseNameServerTransport get timed_mutex timeout");
    return false;
  }
  std::lock_guard<std::timed_mutex> lock(m_namesrvLock, std::adopt_lock);

  std::string addr = m_namesrvAddrChoosed;

  bool removeItemFromTable = CloseTransport(addr, channel);
  if (removeItemFromTable) {
    m_namesrvAddrChoosed.clear();
  }

  return removeItemFromTable;
}

bool TcpRemotingClient::SendCommand(TcpTransportPtr channel, RemotingCommand& msg) {
  MemoryBlockPtr3 package(msg.encode());
  return channel->sendMessage(package->getData(), package->getSize());
}

void TcpRemotingClient::MessageReceived(void* context, MemoryBlockPtr3& mem, const std::string& addr) {
  auto* client = reinterpret_cast<TcpRemotingClient*>(context);
  if (client != nullptr) {
    client->messageReceived(mem, addr);
  }
}

void TcpRemotingClient::messageReceived(MemoryBlockPtr3& mem, const std::string& addr) {
  m_dispatchExecutor.submit(
      std::bind(&TcpRemotingClient::processMessageReceived, this, MemoryBlockPtr2(std::move(mem)), addr));
}

void TcpRemotingClient::processMessageReceived(MemoryBlockPtr2& mem, const std::string& addr) {
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
    // FIXME: memory leak on cmd, if m_handleExecutor is shutdown, but some tasks are unfinished.
    m_handleExecutor.submit(std::bind(&TcpRemotingClient::processRequestCommand, this, cmd, addr));
  }
}

void TcpRemotingClient::processResponseCommand(RemotingCommand* cmd) {
  int opaque = cmd->getOpaque();
  std::shared_ptr<ResponseFuture> responseFuture = findAndDeleteResponseFuture(opaque);
  if (responseFuture != nullptr) {
    int code = responseFuture->getRequestCode();
    LOG_DEBUG("processResponseCommand, opaque:%d, code:%d", opaque, code);

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

void TcpRemotingClient::processRequestCommand(RemotingCommand* cmd, const std::string& addr) {
  std::unique_ptr<RemotingCommand> requestCommand(cmd);
  int requestCode = requestCommand->getCode();
  auto iter = m_processorTable.find(requestCode);
  if (iter != m_processorTable.end()) {
    try {
      auto& processor = iter->second;

      doBeforeRpcHooks(addr, *requestCommand, false);
      std::unique_ptr<RemotingCommand> response(processor->processRequest(addr, requestCommand.get()));
      doAfterRpcHooks(addr, *requestCommand, response.get(), true);

      if (!requestCommand->isOnewayRPC()) {
        if (response != nullptr) {
          response->setOpaque(requestCommand->getOpaque());
          response->markResponseType();

          try {
            TcpTransportPtr channel = GetTransport(addr, true);
            if (channel != nullptr) {
              if (!SendCommand(channel, *response)) {
                LOG_WARN_NEW("send a response command to channel <{}> failed.", channel->getPeerAddrAndPort());
              }
            }
          } catch (std::exception& e) {
            LOG_ERROR_NEW("process request over, but response failed. {}", e.what());
          }
        } else {
        }
      }
    } catch (std::exception& e) {
      LOG_ERROR_NEW("process request exception. {}", e.what());

      if (!cmd->isOnewayRPC()) {
        // TODO: send SYSTEM_ERROR response
      }
    }
  } else {
    std::string error = "request type " + UtilAll::to_string(requestCommand->getCode()) + " not supported";

    // TODO: send REQUEST_CODE_NOT_SUPPORTED response

    LOG_ERROR("{} {}", addr, error);
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

void TcpRemotingClient::registerProcessor(MQRequestCode requestCode, RequestProcessor* requestProcessor) {
  if (m_processorTable.find(requestCode) != m_processorTable.end()) {
    m_processorTable.erase(requestCode);
  }
  m_processorTable[requestCode] = requestProcessor;
}

}  // namespace rocketmq
