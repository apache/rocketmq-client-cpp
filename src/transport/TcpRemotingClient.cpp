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
TcpRemotingClient::TcpRemotingClient(int pullThreadNum, uint64_t tcpConnectTimeout, uint64_t tcpTransportTryLockTimeout)
    : m_pullThreadNum(pullThreadNum),
      m_tcpConnectTimeout(tcpConnectTimeout),
      m_tcpTransportTryLockTimeout(tcpTransportTryLockTimeout),
      m_namesrvIndex(0),
      m_ioServiceWork(m_ioService) {
#if !defined(WIN32) && !defined(__APPLE__)
  string taskName = UtilAll::getProcessName();
  prctl(PR_SET_NAME, "NetworkTP", 0, 0, 0);
#endif
  for (int i = 0; i != m_pullThreadNum; ++i) {
    m_threadpool.create_thread(boost::bind(&boost::asio::io_service::run, &m_ioService));
  }
#if !defined(WIN32) && !defined(__APPLE__)
  prctl(PR_SET_NAME, taskName.c_str(), 0, 0, 0);
#endif

  LOG_INFO("m_tcpConnectTimeout:{}, m_tcpTransportTryLockTimeout:{}, m_pullThreadNum:{}", m_tcpConnectTimeout,
           m_tcpTransportTryLockTimeout, m_pullThreadNum);

  m_async_service_thread.reset(new boost::thread(boost::bind(&TcpRemotingClient::boost_asio_work, this)));
}

void TcpRemotingClient::boost_asio_work() {
  LOG_INFO("TcpRemotingClient::boost asio async service running");

#if !defined(WIN32) && !defined(__APPLE__)
  prctl(PR_SET_NAME, "RemotingAsioT", 0, 0, 0);
#endif

  // avoid async io service stops after first timer timeout callback
  boost::asio::io_service::work work(m_async_ioService);

  m_async_ioService.run();
}

TcpRemotingClient::~TcpRemotingClient() {
  m_tcpTable.clear();
  m_futureTable.clear();
  m_asyncFutureTable.clear();
  m_namesrvAddrList.clear();
  removeAllTimerCallback();
}

void TcpRemotingClient::stopAllTcpTransportThread() {
  LOG_DEBUG("TcpRemotingClient::stopAllTcpTransportThread Begin");

  m_async_ioService.stop();
  m_async_service_thread->interrupt();
  m_async_service_thread->join();
  removeAllTimerCallback();

  {
    for (const auto& trans : m_tcpTable) {
      trans.second->disconnect(trans.first);
    }
    m_tcpTable.clear();
  }

  m_ioService.stop();
  m_threadpool.join_all();

  {
    std::lock_guard<std::mutex> lock(m_futureTableLock);
    for (const auto& future : m_futureTable) {
      if (future.second)
        future.second->releaseThreadCondition();
    }
  }

  LOG_DEBUG("TcpRemotingClient::stopAllTcpTransportThread End");
}

void TcpRemotingClient::updateNameServerAddressList(const string& addrs) {
  LOG_INFO("updateNameServerAddressList: [{}]", addrs.c_str());

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
      LOG_INFO("update Namesrv:{}", addr.c_str());
      m_namesrvAddrList.push_back(addr);
    } else {
      LOG_INFO("This may be invalid namer server: [{}]", addr.c_str());
    }
  }
  out.clear();
}

bool TcpRemotingClient::invokeHeartBeat(const string& addr, RemotingCommand& request, int timeoutMillis) {
  std::shared_ptr<TcpTransport> pTcp = GetTransport(addr, true);
  if (pTcp != nullptr) {
    int code = request.getCode();
    int opaque = request.getOpaque();

    std::shared_ptr<ResponseFuture> responseFuture(new ResponseFuture(code, opaque, this, timeoutMillis));
    addResponseFuture(opaque, responseFuture);

    if (SendCommand(pTcp, request)) {
      responseFuture->setSendRequestOK(true);
      unique_ptr<RemotingCommand> pRsp(responseFuture->waitResponse());
      if (pRsp == nullptr) {
        LOG_ERROR("wait response timeout of heartbeat, so closeTransport of addr:{}", addr.c_str());
        // avoid responseFuture leak;
        findAndDeleteResponseFuture(opaque);
        CloseTransport(addr, pTcp);
        return false;
      } else if (pRsp->getCode() == SUCCESS_VALUE) {
        return true;
      } else {
        LOG_WARN("get error response:{} of heartbeat to addr:{}", pRsp->getCode(), addr.c_str());
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

    std::shared_ptr<ResponseFuture> responseFuture(new ResponseFuture(code, opaque, this, timeoutMillis));
    addResponseFuture(opaque, responseFuture);

    if (SendCommand(pTcp, request)) {
      responseFuture->setSendRequestOK(true);
      RemotingCommand* pRsp = responseFuture->waitResponse();
      if (pRsp == nullptr) {
        if (code != GET_CONSUMER_LIST_BY_GROUP) {
          LOG_WARN("wait response timeout or get NULL response of code:{}, so closeTransport of addr:{}", code,
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
  LOG_DEBUG("InvokeSync [{}] Failed: Cannot Get Transport.", addr.c_str());
  return nullptr;
}

bool TcpRemotingClient::invokeAsync(const string& addr,
                                    RemotingCommand& request,
                                    AsyncCallbackWrap* callback,
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
    addAsyncResponseFuture(opaque, responseFuture);

    if (callback) {
      boost::asio::deadline_timer* t =
          new boost::asio::deadline_timer(m_async_ioService, boost::posix_time::milliseconds(timeoutMillis));
      addTimerCallback(t, opaque);
      t->async_wait(
          boost::bind(&TcpRemotingClient::handleAsyncRequestTimeout, this, boost::asio::placeholders::error, opaque));
    }

    // Even if send failed, asyncTimerThread will trigger next pull request or report send msg failed
    if (SendCommand(pTcp, request)) {
      LOG_DEBUG("invokeAsync success, addr:{}, code:{}, opaque:{}", addr.c_str(), code, opaque);
      responseFuture->setSendRequestOK(true);
    }
    return true;
  }

  LOG_ERROR("invokeAsync failed of addr:{}", addr.c_str());
  return false;
}

void TcpRemotingClient::invokeOneway(const string& addr, RemotingCommand& request) {
  //<!not need callback;
  std::shared_ptr<TcpTransport> pTcp = GetTransport(addr, true);
  if (pTcp != nullptr) {
    request.markOnewayRPC();
    if (SendCommand(pTcp, request)) {
      LOG_DEBUG("invokeOneway success. addr:{}, code:{}", addr.c_str(), request.getCode());
    } else {
      LOG_WARN("invokeOneway failed. addr:{}, code:{}", addr.c_str(), request.getCode());
    }
  } else {
    LOG_WARN("invokeOneway failed: NULL transport. addr:{}, code:{}", addr.c_str(), request.getCode());
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
        LOG_ERROR("GetTransport of:{} get timed_mutex timeout", addr.c_str());
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
          LOG_ERROR("tcpTransport with server disconnected, erase server:{}", addr.c_str());
          tcp->disconnect(addr);  // avoid coredump when connection with broker was broken
          m_tcpTable.erase(addr);
        } else {
          LOG_ERROR("go to fault state, erase:{} from tcpMap, and reconnect it", addr.c_str());
          m_tcpTable.erase(addr);
        }
      }
    }

    //<!callback;
    TcpTransportReadCallback callback = needResponse ? &TcpRemotingClient::static_messageReceived : nullptr;

    tts = TcpTransport::CreateTransport(this, callback);
    TcpConnectStatus connectStatus = tts->connect(addr, 0);  // use non-block
    if (connectStatus != TCP_CONNECT_STATUS_WAIT) {
      LOG_WARN("can not connect to:{}", addr.c_str());
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
    LOG_WARN("can not connect to server:{}", addr.c_str());
    tts->disconnect(addr);
    std::shared_ptr<TcpTransport> pTcp;
    return pTcp;
  } else {
    LOG_INFO("connect server with addr:{} success", addr.c_str());
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

  for (int i = 0; i < m_namesrvAddrList.size(); i++) {
    unsigned int index = m_namesrvIndex++ % m_namesrvAddrList.size();
    LOG_INFO("namesrvIndex is:{}, index:{}, namesrvaddrlist size:" SIZET_FMT "", m_namesrvIndex, index,
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
      LOG_ERROR("CloseTransport of:{} get timed_mutex timeout", addr.c_str());
      return false;
    }
  }

  LOG_ERROR("CloseTransport of:{}", addr.c_str());

  bool removeItemFromTable = true;
  if (m_tcpTable.find(addr) != m_tcpTable.end()) {
    if (m_tcpTable[addr]->getStartTime() != pTcp->getStartTime()) {
      LOG_INFO("tcpTransport with addr:{} has been closed before, and has been created again, nothing to do",
               addr.c_str());
      removeItemFromTable = false;
    }
  } else {
    LOG_INFO("tcpTransport with addr:{} had been removed from tcpTable before", addr.c_str());
    removeItemFromTable = false;
  }

  if (removeItemFromTable) {
    LOG_WARN("closeTransport: disconnect:{} with state:{}", addr.c_str(), m_tcpTable[addr]->getTcpConnectStatus());
    if (m_tcpTable[addr]->getTcpConnectStatus() == TCP_CONNECT_STATUS_SUCCESS)
      m_tcpTable[addr]->disconnect(addr);  // avoid coredump when connection with server was broken
    LOG_WARN("closeTransport: erase broker: {}", addr.c_str());
    m_tcpTable.erase(addr);
  }

  LOG_ERROR("CloseTransport of:{} end", addr.c_str());

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
  m_ioService.post(boost::bind(&TcpRemotingClient::ProcessData, this, mem, addr));
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
    std::shared_ptr<ResponseFuture> pFuture = findAndDeleteAsyncResponseFuture(opaque);
    if (!pFuture) {
      pFuture = findAndDeleteResponseFuture(opaque);
      if (!pFuture) {
        LOG_DEBUG("responseFuture was deleted by timeout of opaque:{}", opaque);
        deleteAndZero(pRespondCmd);
        return;
      }
    }

    LOG_DEBUG("find_response opaque:{}", opaque);
    processResponseCommand(pRespondCmd, pFuture);
  } else {
    processRequestCommand(pRespondCmd, addr);
  }
}

void TcpRemotingClient::processResponseCommand(RemotingCommand* pCmd, std::shared_ptr<ResponseFuture> pFuture) {
  int code = pFuture->getRequestCode();
  pCmd->SetExtHeader(code);  // set head, for response use

  int opaque = pCmd->getOpaque();
  LOG_DEBUG("processResponseCommand, code:{}, opaque:{}, maxRetryTimes:{}, retrySendTimes:{}", code, opaque,
            pFuture->getMaxRetrySendTimes(), pFuture->getRetrySendTimes());

  if (!pFuture->setResponse(pCmd)) {
    // this branch is unreachable normally.
    LOG_WARN("response already timeout of opaque:{}", opaque);
    deleteAndZero(pCmd);
    return;
  }

  if (pFuture->getAsyncFlag()) {
    cancelTimerCallback(opaque);
    pFuture->invokeCompleteCallback();
  }
}

void TcpRemotingClient::handleAsyncRequestTimeout(const boost::system::error_code& e, int opaque) {
  if (e == boost::asio::error::operation_aborted) {
    LOG_DEBUG("handleAsyncRequestTimeout aborted opaque:{}, e_code:{}, msg:{}", opaque, e.value(), e.message().data());
    return;
  }

  LOG_DEBUG("handleAsyncRequestTimeout opaque:{}, e_code:{}, msg:{}", opaque, e.value(), e.message().data());

  std::shared_ptr<ResponseFuture> pFuture(findAndDeleteAsyncResponseFuture(opaque));
  if (pFuture) {
    LOG_ERROR("no response got for opaque:{}", opaque);
    eraseTimerCallback(opaque);
    if (pFuture->getAsyncCallbackWrap()) {
      pFuture->invokeExceptionCallback();
    }
  }
}

void TcpRemotingClient::processRequestCommand(RemotingCommand* pCmd, const string& addr) {
  unique_ptr<RemotingCommand> pRequestCommand(pCmd);
  int requestCode = pRequestCommand->getCode();
  if (m_requestTable.find(requestCode) == m_requestTable.end()) {
    LOG_ERROR("can_not_find request:{} processor", requestCode);
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

void TcpRemotingClient::addAsyncResponseFuture(int opaque, std::shared_ptr<ResponseFuture> pFuture) {
  std::lock_guard<std::mutex> lock(m_asyncFutureTableLock);
  m_asyncFutureTable[opaque] = pFuture;
}

// Note: after call this function, shared_ptr of m_asyncFutureTable[opaque] will
// be erased, so caller must ensure the life cycle of returned shared_ptr;
std::shared_ptr<ResponseFuture> TcpRemotingClient::findAndDeleteAsyncResponseFuture(int opaque) {
  std::lock_guard<std::mutex> lock(m_asyncFutureTableLock);
  std::shared_ptr<ResponseFuture> pResponseFuture;
  if (m_asyncFutureTable.find(opaque) != m_asyncFutureTable.end()) {
    pResponseFuture = m_asyncFutureTable[opaque];
    m_asyncFutureTable.erase(opaque);
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
    LOG_DEBUG("addTimerCallback:erase timerCallback opaque:{}", opaque);
    boost::asio::deadline_timer* old_t = m_asyncTimerTable[opaque];
    m_asyncTimerTable.erase(opaque);
    try {
      old_t->cancel();
    } catch (const std::exception& ec) {
      LOG_WARN("encounter exception when cancel old timer: {}", ec.what());
    }
    delete old_t;
  }
  m_asyncTimerTable[opaque] = t;
}

void TcpRemotingClient::eraseTimerCallback(int opaque) {
  std::lock_guard<std::mutex> lock(m_asyncTimerTableLock);
  if (m_asyncTimerTable.find(opaque) != m_asyncTimerTable.end()) {
    LOG_DEBUG("eraseTimerCallback: opaque:{}", opaque);
    boost::asio::deadline_timer* t = m_asyncTimerTable[opaque];
    m_asyncTimerTable.erase(opaque);
    delete t;
  }
}

void TcpRemotingClient::cancelTimerCallback(int opaque) {
  std::lock_guard<std::mutex> lock(m_asyncTimerTableLock);
  if (m_asyncTimerTable.find(opaque) != m_asyncTimerTable.end()) {
    LOG_DEBUG("cancelTimerCallback: opaque:{}", opaque);
    boost::asio::deadline_timer* t = m_asyncTimerTable[opaque];
    m_asyncTimerTable.erase(opaque);
    try {
      t->cancel();
    } catch (const std::exception& ec) {
      LOG_WARN("encounter exception when cancel timer: {}", ec.what());
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
      LOG_WARN("encounter exception when cancel timer: {}", ec.what());
    }
    delete t;
  }
  m_asyncTimerTable.clear();
}

void TcpRemotingClient::deleteOpaqueForDropPullRequest(const MQMessageQueue& mq, int opaque) {
  // delete the map record of opaque<->ResponseFuture, so the answer for the pull request will
  // discard when receive it later
  std::shared_ptr<ResponseFuture> pFuture(findAndDeleteAsyncResponseFuture(opaque));
  if (!pFuture) {
    pFuture = findAndDeleteResponseFuture(opaque);
    if (pFuture) {
      LOG_DEBUG("succ deleted the sync pullrequest for opaque:{}, mq:{}", opaque, mq.toString().data());
    }
  } else {
    LOG_DEBUG("succ deleted the async pullrequest for opaque:{}, mq:{}", opaque, mq.toString().data());
    // delete the timeout timer for opaque for pullrequest
    cancelTimerCallback(opaque);
  }
}

//<!************************************************************************
}  // namespace rocketmq
