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
#include "TopAddressing.h"
#include "UtilAll.h"

namespace rocketmq {

//<!************************************************************************
TcpRemotingClient::TcpRemotingClient(bool enableSsl, const std::string& sslPropertyFile)
    : m_enableSsl(enableSsl),
      m_sslPropertyFile(sslPropertyFile),
      m_dispatchServiceWork(m_dispatchService),
      m_handleServiceWork(m_handleService) {}
TcpRemotingClient::TcpRemotingClient(int pullThreadNum,
                                     uint64_t tcpConnectTimeout,
                                     uint64_t tcpTransportTryLockTimeout,
                                     bool enableSsl,
                                     const std::string& sslPropertyFile)
    : m_dispatchThreadNum(1),
      m_pullThreadNum(pullThreadNum),
      m_tcpConnectTimeout(tcpConnectTimeout),
      m_tcpTransportTryLockTimeout(tcpTransportTryLockTimeout),
      m_enableSsl(enableSsl),
      m_sslPropertyFile(sslPropertyFile),
      m_namesrvIndex(0),
      m_dispatchServiceWork(m_dispatchService),
      m_handleServiceWork(m_handleService) {
#if !defined(WIN32) && !defined(__APPLE__)
  string taskName = UtilAll::getProcessName();
  prctl(PR_SET_NAME, "DispatchTP", 0, 0, 0);
#endif
  for (int i = 0; i != m_dispatchThreadNum; ++i) {
    m_dispatchThreadPool.create_thread(boost::bind(&boost::asio::io_service::run, &m_dispatchService));
  }
#if !defined(WIN32) && !defined(__APPLE__)
  prctl(PR_SET_NAME, taskName.c_str(), 0, 0, 0);
#endif

#if !defined(WIN32) && !defined(__APPLE__)
  prctl(PR_SET_NAME, "NetworkTP", 0, 0, 0);
#endif
  for (int i = 0; i != m_pullThreadNum; ++i) {
    m_handleThreadPool.create_thread(boost::bind(&boost::asio::io_service::run, &m_handleService));
  }
#if !defined(WIN32) && !defined(__APPLE__)
  prctl(PR_SET_NAME, taskName.c_str(), 0, 0, 0);
#endif

  LOG_INFO("m_tcpConnectTimeout:%ju, m_tcpTransportTryLockTimeout:%ju, m_pullThreadNum:%d", m_tcpConnectTimeout,
           m_tcpTransportTryLockTimeout, m_pullThreadNum);

  m_timerServiceThread.reset(new boost::thread(boost::bind(&TcpRemotingClient::boost_asio_work, this)));
}

void TcpRemotingClient::boost_asio_work() {
  LOG_INFO("TcpRemotingClient::boost asio async service running");

#if !defined(WIN32) && !defined(__APPLE__)
  prctl(PR_SET_NAME, "RemotingAsioT", 0, 0, 0);
#endif

  // avoid async io service stops after first timer timeout callback
  boost::asio::io_service::work work(m_timerService);

  m_timerService.run();
}

TcpRemotingClient::~TcpRemotingClient() {
  m_tcpTable.clear();
  m_futureTable.clear();
  m_namesrvAddrList.clear();
  removeAllTimerCallback();
}

void TcpRemotingClient::stopAllTcpTransportThread() {
  LOG_DEBUG("TcpRemotingClient::stopAllTcpTransportThread Begin");

  m_timerService.stop();
  m_timerServiceThread->interrupt();
  m_timerServiceThread->join();
  removeAllTimerCallback();

  {
    std::lock_guard<std::timed_mutex> lock(m_tcpTableLock);
    for (const auto& trans : m_tcpTable) {
      trans.second->disconnect(trans.first);
    }
    m_tcpTable.clear();
  }

  m_handleService.stop();
  m_handleThreadPool.join_all();

  m_dispatchService.stop();
  m_dispatchThreadPool.join_all();

  {
    std::lock_guard<std::mutex> lock(m_futureTableLock);
    for (const auto& future : m_futureTable) {
      if (future.second) {
        if (!future.second->getAsyncFlag()) {
          future.second->releaseThreadCondition();
        }
      }
    }
  }

  LOG_ERROR("TcpRemotingClient::stopAllTcpTransportThread End, m_tcpTable:%lu", m_tcpTable.size());
}

void TcpRemotingClient::updateNameServerAddressList(const string& addrs) {
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

  vector<string> out;
  UtilAll::Split(out, addrs, ";");
  for (auto addr : out) {
    UtilAll::Trim(addr);

    string hostName;
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

bool TcpRemotingClient::invokeHeartBeat(const string& addr, RemotingCommand& request, int timeoutMillis) {
  std::shared_ptr<TcpTransport> pTcp = GetTransport(addr, true);
  if (pTcp != nullptr) {
    int code = request.getCode();
    int opaque = request.getOpaque();
    std::shared_ptr<AsyncCallbackWrap> cbw;
    std::shared_ptr<ResponseFuture> responseFuture(new ResponseFuture(code, opaque, this, timeoutMillis, false, cbw));
    addResponseFuture(opaque, responseFuture);

    if (SendCommand(pTcp, request)) {
      responseFuture->setSendRequestOK(true);
      unique_ptr<RemotingCommand> pRsp(responseFuture->waitResponse());
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

RemotingCommand* TcpRemotingClient::invokeSync(const string& addr, RemotingCommand& request, int timeoutMillis) {
  LOG_DEBUG("InvokeSync:", addr.c_str());
  std::shared_ptr<TcpTransport> pTcp = GetTransport(addr, true);
  if (pTcp != nullptr) {
    int code = request.getCode();
    int opaque = request.getOpaque();
    std::shared_ptr<AsyncCallbackWrap> cbw;
    std::shared_ptr<ResponseFuture> responseFuture(new ResponseFuture(code, opaque, this, timeoutMillis, false, cbw));
    addResponseFuture(opaque, responseFuture);

    if (SendCommand(pTcp, request)) {
      responseFuture->setSendRequestOK(true);
      RemotingCommand* pRsp = responseFuture->waitResponse();
      if (pRsp == nullptr) {
        if (code != GET_CONSUMER_LIST_BY_GROUP) {
          LOG_WARN("wait response timeout or get NULL response of code:%d, so closeTransport of addr:%s", code,
                   addr.c_str());
          CloseTransport(addr, pTcp);
        }
        // avoid responseFuture leak;
        findAndDeleteResponseFuture(opaque);
        return nullptr;
      } else {
        return pRsp;
      }
    } else {
      // avoid responseFuture leak;
      findAndDeleteResponseFuture(opaque);
      CloseTransport(addr, pTcp);
    }
  }
  LOG_DEBUG("InvokeSync [%s] Failed: Cannot Get Transport.", addr.c_str());
  return nullptr;
}

bool TcpRemotingClient::invokeAsync(const string& addr,
                                    RemotingCommand& request,
                                    std::shared_ptr<AsyncCallbackWrap> callback,
                                    int64 timeoutMillis,
                                    int maxRetrySendTimes,
                                    int retrySendTimes) {
  std::shared_ptr<TcpTransport> pTcp = GetTransport(addr, true);
  if (pTcp != nullptr) {
    int code = request.getCode();
    int opaque = request.getOpaque();

    // delete in callback
    std::shared_ptr<ResponseFuture> responseFuture(
        new ResponseFuture(code, opaque, this, timeoutMillis, true, callback));
    responseFuture->setMaxRetrySendTimes(maxRetrySendTimes);
    responseFuture->setRetrySendTimes(retrySendTimes);
    responseFuture->setBrokerAddr(addr);
    responseFuture->setRequestCommand(request);
    addResponseFuture(opaque, responseFuture);

    // timeout monitor
    boost::asio::deadline_timer* t =
        new boost::asio::deadline_timer(m_timerService, boost::posix_time::milliseconds(timeoutMillis));
    addTimerCallback(t, opaque);
    t->async_wait(
        boost::bind(&TcpRemotingClient::handleAsyncRequestTimeout, this, boost::asio::placeholders::error, opaque));

    // even if send failed, asyncTimerThread will trigger next pull request or report send msg failed
    if (SendCommand(pTcp, request)) {
      LOG_DEBUG("invokeAsync success, addr:%s, code:%d, opaque:%d", addr.c_str(), code, opaque);
      responseFuture->setSendRequestOK(true);
    }
    return true;
  }

  LOG_ERROR("invokeAsync failed of addr:%s", addr.c_str());
  return false;
}

void TcpRemotingClient::invokeOneway(const string& addr, RemotingCommand& request) {
  //<!not need callback;
  std::shared_ptr<TcpTransport> pTcp = GetTransport(addr, true);
  if (pTcp != nullptr) {
    request.markOnewayRPC();
    if (SendCommand(pTcp, request)) {
      LOG_DEBUG("invokeOneway success. addr:%s, code:%d", addr.c_str(), request.getCode());
    } else {
      LOG_WARN("invokeOneway failed. addr:%s, code:%d", addr.c_str(), request.getCode());
    }
  } else {
    LOG_WARN("invokeOneway failed: NULL transport. addr:%s, code:%d", addr.c_str(), request.getCode());
  }
}

std::shared_ptr<TcpTransport> TcpRemotingClient::GetTransport(const string& addr, bool needResponse) {
  if (addr.empty()) {
    LOG_DEBUG("GetTransport of NameServer");
    return CreateNameServerTransport(needResponse);
  }
  return CreateTransport(addr, needResponse);
}

std::shared_ptr<TcpTransport> TcpRemotingClient::CreateTransport(const string& addr, bool needResponse) {
  std::shared_ptr<TcpTransport> tts;

  {
    // try get m_tcpLock util m_tcpTransportTryLockTimeout to avoid blocking
    // long time, if could not get m_tcpLock, return NULL
    std::unique_lock<std::timed_mutex> lock(m_tcpTableLock, std::try_to_lock);
    if (!lock.owns_lock()) {
      if (!lock.try_lock_for(std::chrono::seconds(m_tcpTransportTryLockTimeout))) {
        LOG_ERROR("GetTransport of:%s get timed_mutex timeout", addr.c_str());
        std::shared_ptr<TcpTransport> pTcp;
        return pTcp;
      }
    }

    // check for reuse
    if (m_tcpTable.find(addr) != m_tcpTable.end()) {
      std::shared_ptr<TcpTransport> tcp = m_tcpTable[addr];

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
          m_tcpTable.erase(addr);
        } else {
          LOG_ERROR("go to fault state, erase:%s from tcpMap, and reconnect it", addr.c_str());
          m_tcpTable.erase(addr);
        }
      }
    }

    //<!callback;
    TcpTransportReadCallback callback = needResponse ? &TcpRemotingClient::static_messageReceived : nullptr;

    tts = TcpTransport::CreateTransport(this, m_enableSsl, m_sslPropertyFile, callback);
    TcpConnectStatus connectStatus = tts->connect(addr, 0);  // use non-block
    if (connectStatus != TCP_CONNECT_STATUS_WAIT) {
      LOG_WARN("can not connect to:%s", addr.c_str());
      tts->disconnect(addr);
      std::shared_ptr<TcpTransport> pTcp;
      return pTcp;
    } else {
      // even if connecting failed finally, this server transport will be erased by next CreateTransport
      m_tcpTable[addr] = tts;
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

bool TcpRemotingClient::CloseTransport(const string& addr, std::shared_ptr<TcpTransport> pTcp) {
  if (addr.empty()) {
    return CloseNameServerTransport(pTcp);
  }

  std::unique_lock<std::timed_mutex> lock(m_tcpTableLock, std::try_to_lock);
  if (!lock.owns_lock()) {
    if (!lock.try_lock_for(std::chrono::seconds(m_tcpTransportTryLockTimeout))) {
      LOG_ERROR("CloseTransport of:%s get timed_mutex timeout", addr.c_str());
      return false;
    }
  }

  LOG_ERROR("CloseTransport of:%s", addr.c_str());

  bool removeItemFromTable = true;
  if (m_tcpTable.find(addr) != m_tcpTable.end()) {
    if (m_tcpTable[addr]->getStartTime() != pTcp->getStartTime()) {
      LOG_INFO("tcpTransport with addr:%s has been closed before, and has been created again, nothing to do",
               addr.c_str());
      removeItemFromTable = false;
    }
  } else {
    LOG_INFO("tcpTransport with addr:%s had been removed from tcpTable before", addr.c_str());
    removeItemFromTable = false;
  }

  if (removeItemFromTable) {
    LOG_WARN("closeTransport: disconnect:%s with state:%d", addr.c_str(), m_tcpTable[addr]->getTcpConnectStatus());
    if (m_tcpTable[addr]->getTcpConnectStatus() == TCP_CONNECT_STATUS_SUCCESS)
      m_tcpTable[addr]->disconnect(addr);  // avoid coredump when connection with server was broken
    LOG_WARN("closeTransport: erase broker: %s", addr.c_str());
    m_tcpTable.erase(addr);
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

  string addr = m_namesrvAddrChoosed;

  bool removeItemFromTable = CloseTransport(addr, pTcp);
  if (removeItemFromTable) {
    m_namesrvAddrChoosed.clear();
  }

  return removeItemFromTable;
}

bool TcpRemotingClient::SendCommand(std::shared_ptr<TcpTransport> pTts, RemotingCommand& msg) {
  const MemoryBlock* pHead = msg.GetHead();
  const MemoryBlock* pBody = msg.GetBody();

  unique_ptr<MemoryOutputStream> buffer(new MemoryOutputStream(1024));
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

void TcpRemotingClient::static_messageReceived(void* context, const MemoryBlock& mem, const string& addr) {
  auto* pTcpRemotingClient = reinterpret_cast<TcpRemotingClient*>(context);
  if (pTcpRemotingClient)
    pTcpRemotingClient->messageReceived(mem, addr);
}

void TcpRemotingClient::messageReceived(const MemoryBlock& mem, const string& addr) {
  m_dispatchService.post(boost::bind(&TcpRemotingClient::ProcessData, this, mem, addr));
}

void TcpRemotingClient::ProcessData(const MemoryBlock& mem, const string& addr) {
  RemotingCommand* pRespondCmd = nullptr;
  try {
    pRespondCmd = RemotingCommand::Decode(mem);
  } catch (...) {
    LOG_ERROR("processData error");
    return;
  }

  int opaque = pRespondCmd->getOpaque();

  //<!process self;
  if (pRespondCmd->isResponseType()) {
    std::shared_ptr<ResponseFuture> pFuture = findAndDeleteResponseFuture(opaque);
    if (!pFuture) {
      LOG_DEBUG("responseFuture was deleted by timeout of opaque:%d", opaque);
      deleteAndZero(pRespondCmd);
      return;
    }

    LOG_DEBUG("find_response opaque:%d", opaque);
    processResponseCommand(pRespondCmd, pFuture);
  } else {
    m_handleService.post(boost::bind(&TcpRemotingClient::processRequestCommand, this, pRespondCmd, addr));
  }
}

void TcpRemotingClient::processResponseCommand(RemotingCommand* pCmd, std::shared_ptr<ResponseFuture> pFuture) {
  int code = pFuture->getRequestCode();
  pCmd->SetExtHeader(code);  // set head, for response use

  int opaque = pCmd->getOpaque();
  LOG_DEBUG("processResponseCommand, code:%d, opaque:%d, maxRetryTimes:%d, retrySendTimes:%d", code, opaque,
            pFuture->getMaxRetrySendTimes(), pFuture->getRetrySendTimes());

  if (!pFuture->setResponse(pCmd)) {
    // this branch is unreachable normally.
    LOG_WARN("response already timeout of opaque:%d", opaque);
    deleteAndZero(pCmd);
    return;
  }

  if (pFuture->getAsyncFlag()) {
    cancelTimerCallback(opaque);

    m_handleService.post(boost::bind(&ResponseFuture::invokeCompleteCallback, pFuture));
  }
}

void TcpRemotingClient::handleAsyncRequestTimeout(const boost::system::error_code& e, int opaque) {
  if (e == boost::asio::error::operation_aborted) {
    LOG_DEBUG("handleAsyncRequestTimeout aborted opaque:%d, e_code:%d, msg:%s", opaque, e.value(), e.message().data());
    return;
  }

  LOG_DEBUG("handleAsyncRequestTimeout opaque:%d, e_code:%d, msg:%s", opaque, e.value(), e.message().data());

  std::shared_ptr<ResponseFuture> pFuture(findAndDeleteResponseFuture(opaque));
  if (pFuture) {
    LOG_ERROR("no response got for opaque:%d", opaque);
    eraseTimerCallback(opaque);
    if (pFuture->getAsyncCallbackWrap()) {
      m_handleService.post(boost::bind(&ResponseFuture::invokeExceptionCallback, pFuture));
    }
  }
}

void TcpRemotingClient::processRequestCommand(RemotingCommand* pCmd, const string& addr) {
  unique_ptr<RemotingCommand> pRequestCommand(pCmd);
  int requestCode = pRequestCommand->getCode();
  if (m_requestTable.find(requestCode) == m_requestTable.end()) {
    LOG_ERROR("can_not_find request:%d processor", requestCode);
  } else {
    unique_ptr<RemotingCommand> pResponse(m_requestTable[requestCode]->processRequest(addr, pRequestCommand.get()));
    if (!pRequestCommand->isOnewayRPC()) {
      if (pResponse) {
        pResponse->setOpaque(pRequestCommand->getOpaque());
        pResponse->markResponseType();
        pResponse->Encode();

        invokeOneway(addr, *pResponse);
      }
    }
  }
}

void TcpRemotingClient::addResponseFuture(int opaque, std::shared_ptr<ResponseFuture> pFuture) {
  std::lock_guard<std::mutex> lock(m_futureTableLock);
  m_futureTable[opaque] = pFuture;
}

// Note: after call this function, shared_ptr of m_syncFutureTable[opaque] will
// be erased, so caller must ensure the life cycle of returned shared_ptr;
std::shared_ptr<ResponseFuture> TcpRemotingClient::findAndDeleteResponseFuture(int opaque) {
  std::lock_guard<std::mutex> lock(m_futureTableLock);
  std::shared_ptr<ResponseFuture> pResponseFuture;
  if (m_futureTable.find(opaque) != m_futureTable.end()) {
    pResponseFuture = m_futureTable[opaque];
    m_futureTable.erase(opaque);
  }
  return pResponseFuture;
}

void TcpRemotingClient::registerProcessor(MQRequestCode requestCode, ClientRemotingProcessor* clientRemotingProcessor) {
  if (m_requestTable.find(requestCode) != m_requestTable.end())
    m_requestTable.erase(requestCode);
  m_requestTable[requestCode] = clientRemotingProcessor;
}

void TcpRemotingClient::addTimerCallback(boost::asio::deadline_timer* t, int opaque) {
  std::lock_guard<std::mutex> lock(m_asyncTimerTableLock);
  if (m_asyncTimerTable.find(opaque) != m_asyncTimerTable.end()) {
    LOG_DEBUG("addTimerCallback:erase timerCallback opaque:%lld", opaque);
    boost::asio::deadline_timer* old_t = m_asyncTimerTable[opaque];
    m_asyncTimerTable.erase(opaque);
    try {
      old_t->cancel();
    } catch (const std::exception& ec) {
      LOG_WARN("encounter exception when cancel old timer: %s", ec.what());
    }
    delete old_t;
  }
  m_asyncTimerTable[opaque] = t;
}

void TcpRemotingClient::eraseTimerCallback(int opaque) {
  std::lock_guard<std::mutex> lock(m_asyncTimerTableLock);
  if (m_asyncTimerTable.find(opaque) != m_asyncTimerTable.end()) {
    LOG_DEBUG("eraseTimerCallback: opaque:%lld", opaque);
    boost::asio::deadline_timer* t = m_asyncTimerTable[opaque];
    m_asyncTimerTable.erase(opaque);
    delete t;
  }
}

void TcpRemotingClient::cancelTimerCallback(int opaque) {
  std::lock_guard<std::mutex> lock(m_asyncTimerTableLock);
  if (m_asyncTimerTable.find(opaque) != m_asyncTimerTable.end()) {
    LOG_DEBUG("cancelTimerCallback: opaque:%lld", opaque);
    boost::asio::deadline_timer* t = m_asyncTimerTable[opaque];
    m_asyncTimerTable.erase(opaque);
    try {
      t->cancel();
    } catch (const std::exception& ec) {
      LOG_WARN("encounter exception when cancel timer: %s", ec.what());
    }
    delete t;
  }
}

void TcpRemotingClient::removeAllTimerCallback() {
  std::lock_guard<std::mutex> lock(m_asyncTimerTableLock);
  for (const auto& timer : m_asyncTimerTable) {
    boost::asio::deadline_timer* t = timer.second;
    try {
      t->cancel();
    } catch (const std::exception& ec) {
      LOG_WARN("encounter exception when cancel timer: %s", ec.what());
    }
    delete t;
  }
  m_asyncTimerTable.clear();
}

//<!************************************************************************
}  // namespace rocketmq
