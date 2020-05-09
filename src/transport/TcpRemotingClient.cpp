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

class ResponseFutureInfo : public TcpTransportInfo {
 public:
  using FutureMap = std::map<int, ResponseFuturePtr>;

 public:
  ~ResponseFutureInfo() {
    // FIXME: Can optimize?
    activeAllResponses();
  }

  void putResponseFuture(int opaque, ResponseFuturePtr future) {
    std::lock_guard<std::mutex> lock(m_futureTableMutex);
    const auto it = m_futureTable.find(opaque);
    if (it != m_futureTable.end()) {
      LOG_WARN_NEW("[BUG] response futurn with opaque:{} is exist.", opaque);
    }
    m_futureTable[opaque] = future;
  }

  ResponseFuturePtr popResponseFuture(int opaque) {
    std::lock_guard<std::mutex> lock(m_futureTableMutex);
    const auto& it = m_futureTable.find(opaque);
    if (it != m_futureTable.end()) {
      auto future = it->second;
      m_futureTable.erase(it);
      return future;
    }
    return nullptr;
  }

  void scanResponseTable(uint64_t now, std::vector<ResponseFuturePtr>& futureList) {
    std::lock_guard<std::mutex> lock(m_futureTableMutex);
    for (auto it = m_futureTable.begin(); it != m_futureTable.end();) {
      auto& future = it->second;  // NOTE: future is a reference
      if (future->getBeginTimestamp() + future->getTimeoutMillis() + 1000 <= now) {
        LOG_WARN_NEW("remove timeout request, code:{}, opaque:{}", future->getRequestCode(), future->getOpaque());
        futureList.push_back(future);
        it = m_futureTable.erase(it);
      } else {
        ++it;
      }
    }
  }

  void activeAllResponses() {
    std::lock_guard<std::mutex> lock(m_futureTableMutex);
    for (const auto& it : m_futureTable) {
      auto& future = it.second;
      if (future->hasInvokeCallback()) {
        future->executeInvokeCallback();  // callback async request
      } else {
        future->putResponse(nullptr);  // wake up sync request
      }
    }
    m_futureTable.clear();
  }

 private:
  FutureMap m_futureTable;  // opaque -> future
  std::mutex m_futureTableMutex;
};

TcpRemotingClient::TcpRemotingClient(int workerThreadNum,
                                     uint64_t tcpConnectTimeout,
                                     uint64_t tcpTransportTryLockTimeout)
    : m_tcpConnectTimeout(tcpConnectTimeout),
      m_tcpTransportTryLockTimeout(tcpTransportTryLockTimeout),
      m_namesrvIndex(0),
      m_dispatchExecutor("MessageDispatchExecutor", 1, false),
      m_handleExecutor("MessageHandleExecutor", workerThreadNum, false),
      m_timeoutExecutor("TimeoutScanExecutor", false) {}

TcpRemotingClient::~TcpRemotingClient() {
  LOG_DEBUG_NEW("TcpRemotingClient is destructed.");
}

void TcpRemotingClient::start() {
  m_dispatchExecutor.startup();
  m_handleExecutor.startup();

  LOG_INFO_NEW("tcpConnectTimeout:{}, tcpTransportTryLockTimeout:{}, pullThreadNum:{}", m_tcpConnectTimeout,
               m_tcpTransportTryLockTimeout, m_handleExecutor.num_threads());

  m_timeoutExecutor.startup();

  // scanResponseTable
  m_timeoutExecutor.schedule(std::bind(&TcpRemotingClient::scanResponseTablePeriodically, this), 1000 * 3,
                             time_unit::milliseconds);
}

void TcpRemotingClient::shutdown() {
  LOG_INFO_NEW("TcpRemotingClient::shutdown Begin");

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

  LOG_INFO_NEW("TcpRemotingClient::shutdown End, m_transportTable:{}", m_transportTable.size());
}

void TcpRemotingClient::registerRPCHook(RPCHookPtr rpcHook) {
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
  LOG_INFO_NEW("updateNameServerAddressList: [{}]", addrs);

  if (addrs.empty()) {
    return;
  }

  if (!UtilAll::try_lock_for(m_namesrvLock, 1000 * 10)) {
    LOG_ERROR_NEW("updateNameServerAddressList get timed_mutex timeout");
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
      LOG_INFO_NEW("update Namesrv:{}", addr);
      m_namesrvAddrList.push_back(addr);
    } else {
      LOG_INFO_NEW("This may be invalid namer server: [{}]", addr);
    }
  }
}

void TcpRemotingClient::scanResponseTablePeriodically() {
  try {
    scanResponseTable();
  } catch (std::exception& e) {
    LOG_ERROR_NEW("scanResponseTable exception: {}", e.what());
  }

  // next round
  m_timeoutExecutor.schedule(std::bind(&TcpRemotingClient::scanResponseTablePeriodically, this), 1000,
                             time_unit::milliseconds);
}

void TcpRemotingClient::scanResponseTable() {
  std::vector<TcpTransportPtr> channelList;
  {
    std::lock_guard<std::timed_mutex> lock(m_transportTableMutex);
    for (const auto& trans : m_transportTable) {
      channelList.push_back(trans.second);
    }
  }

  auto now = UtilAll::currentTimeMillis();

  std::vector<ResponseFuturePtr> rfList;
  for (auto channel : channelList) {
    static_cast<ResponseFutureInfo*>(channel->getInfo())->scanResponseTable(now, rfList);
  }
  for (auto rf : rfList) {
    if (rf->hasInvokeCallback()) {
      m_handleExecutor.submit(std::bind(&ResponseFuture::executeInvokeCallback, rf));
    }
  }
}

std::unique_ptr<RemotingCommand> TcpRemotingClient::invokeSync(const std::string& addr,
                                                               RemotingCommand& request,
                                                               int timeoutMillis) throw(RemotingException) {
  auto beginStartTime = UtilAll::currentTimeMillis();
  auto channel = GetTransport(addr);
  if (channel != nullptr) {
    try {
      doBeforeRpcHooks(addr, request, true);
      auto costTime = UtilAll::currentTimeMillis() - beginStartTime;
      if (timeoutMillis <= 0 || timeoutMillis < costTime) {
        THROW_MQEXCEPTION(RemotingTimeoutException, "invokeSync call timeout", -1);
      }
      std::unique_ptr<RemotingCommand> response(invokeSyncImpl(channel, request, timeoutMillis));
      doAfterRpcHooks(addr, request, response.get(), false);
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

std::unique_ptr<RemotingCommand> TcpRemotingClient::invokeSyncImpl(
    TcpTransportPtr channel,
    RemotingCommand& request,
    int64_t timeoutMillis) throw(RemotingTimeoutException, RemotingSendRequestException) {
  int code = request.getCode();
  int opaque = request.getOpaque();

  auto responseFuture = std::make_shared<ResponseFuture>(code, opaque, timeoutMillis);
  putResponseFuture(channel, opaque, responseFuture);

  if (SendCommand(channel, request)) {
    responseFuture->setSendRequestOK(true);
  } else {
    responseFuture->setSendRequestOK(false);
    popResponseFuture(channel, opaque);  // clean future
    // wake up potential waitResponse() is unnecessary, so not call putResponse()
    LOG_WARN_NEW("send a request command to channel <{}> failed.", channel->getPeerAddrAndPort());
    THROW_MQEXCEPTION(RemotingSendRequestException, "send request to <" + channel->getPeerAddrAndPort() + "> failed",
                      -1);
  }

  std::unique_ptr<RemotingCommand> response(responseFuture->waitResponse(timeoutMillis));
  if (nullptr == response) {             // timeout
    popResponseFuture(channel, opaque);  // clean future
    THROW_MQEXCEPTION(RemotingTimeoutException,
                      "wait response on the addr <" + channel->getPeerAddrAndPort() + "> timeout", -1);
  }

  return response;
}

void TcpRemotingClient::invokeAsync(const std::string& addr,
                                    RemotingCommand& request,
                                    InvokeCallback* invokeCallback,
                                    int64_t timeoutMillis) throw(RemotingException) {
  auto beginStartTime = UtilAll::currentTimeMillis();
  auto channel = GetTransport(addr);
  if (channel != nullptr) {
    try {
      doBeforeRpcHooks(addr, request, true);
      auto costTime = UtilAll::currentTimeMillis() - beginStartTime;
      if (timeoutMillis <= 0 || timeoutMillis < costTime) {
        THROW_MQEXCEPTION(RemotingTooMuchRequestException, "invokeAsync call timeout", -1);
      }
      invokeAsyncImpl(channel, request, timeoutMillis, invokeCallback);
    } catch (const RemotingSendRequestException& e) {
      LOG_WARN_NEW("invokeAsync: send request exception, so close the channel[{}]", channel->getPeerAddrAndPort());
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
  auto responseFuture = std::make_shared<ResponseFuture>(code, opaque, timeoutMillis, invokeCallback);
  putResponseFuture(channel, opaque, responseFuture);

  try {
    if (SendCommand(channel, request)) {
      responseFuture->setSendRequestOK(true);
    } else {
      // requestFail
      responseFuture = popResponseFuture(channel, opaque);
      if (responseFuture != nullptr) {
        responseFuture->setSendRequestOK(false);
        if (responseFuture->hasInvokeCallback()) {
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
  auto channel = GetTransport(addr);
  if (channel != nullptr) {
    try {
      doBeforeRpcHooks(addr, request, true);
      invokeOnewayImpl(channel, request);
    } catch (const RemotingSendRequestException& e) {
      LOG_WARN_NEW("invokeOneway: send request exception, so close the channel[{}]", channel->getPeerAddrAndPort());
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

TcpTransportPtr TcpRemotingClient::GetTransport(const std::string& addr) {
  if (addr.empty()) {
    LOG_DEBUG_NEW("Get namesrv transport");
    return CreateNameServerTransport();
  }
  return CreateTransport(addr);
}

TcpTransportPtr TcpRemotingClient::CreateTransport(const std::string& addr) {
  TcpTransportPtr channel;

  {
    // try get m_transportTableMutex until m_tcpTransportTryLockTimeout to avoid blocking long time,
    // if could not get m_transportTableMutex, return NULL
    if (!UtilAll::try_lock_for(m_transportTableMutex, 1000 * m_tcpTransportTryLockTimeout)) {
      LOG_ERROR_NEW("GetTransport of:{} get timed_mutex timeout", addr);
      return nullptr;
    }
    std::lock_guard<std::timed_mutex> lock(m_transportTableMutex, std::adopt_lock);

    // check for reuse
    const auto& it = m_transportTable.find(addr);
    if (it != m_transportTable.end()) {
      channel = it->second;
    }

    if (channel != nullptr) {
      TcpConnectStatus connectStatus = channel->getTcpConnectStatus();
      switch (connectStatus) {
        // case TCP_CONNECT_STATUS_CREATED:
        case TCP_CONNECT_STATUS_CONNECTED:
          return channel;
        case TCP_CONNECT_STATUS_CONNECTING:
          // wait server answer
          break;
        case TCP_CONNECT_STATUS_FAILED:
          LOG_ERROR_NEW("tcpTransport with server disconnected, erase server:{}", addr);
          channel->disconnect(addr);
          m_transportTable.erase(it);
          break;
        default:  // TCP_CONNECT_STATUS_CLOSED
          LOG_ERROR_NEW("go to CLOSED state, erase:{} from transportTable, and reconnect it", addr);
          m_transportTable.erase(it);
          break;
      }
    } else {
      // callback
      TcpTransport::ReadCallback readCallback =
          std::bind(&TcpRemotingClient::messageReceived, this, std::placeholders::_1, std::placeholders::_2);
      TcpTransport::CloseCallback closeCallback =
          std::bind(&TcpRemotingClient::channelClosed, this, std::placeholders::_1);

      // create new transport, then connect server
      std::unique_ptr<ResponseFutureInfo> responseFutureInfo(new ResponseFutureInfo());
      channel = TcpTransport::CreateTransport(readCallback, closeCallback, std::move(responseFutureInfo));
      TcpConnectStatus connectStatus = channel->connect(addr, 0);  // use non-block
      if (connectStatus != TCP_CONNECT_STATUS_CONNECTING) {
        LOG_WARN_NEW("can not connect to:{}", addr);
        channel->disconnect(addr);
        return nullptr;
      } else {
        // even if connecting failed finally, this server transport will be erased by next CreateTransport
        m_transportTable[addr] = channel;
      }
    }
  }

  // waiting...
  TcpConnectStatus connectStatus = channel->waitTcpConnectEvent(static_cast<int>(m_tcpConnectTimeout));
  if (connectStatus != TCP_CONNECT_STATUS_CONNECTED) {
    LOG_WARN_NEW("can not connect to server:{}", addr);
    // channel->disconnect(addr);
    return nullptr;
  } else {
    LOG_INFO_NEW("connect server with addr:{} success", addr);
    return channel;
  }
}

TcpTransportPtr TcpRemotingClient::CreateNameServerTransport() {
  // m_namesrvLock was added to avoid operation of NameServer was blocked by
  // m_tcpLock, it was used by single Thread mostly, so no performance impact
  // try get m_tcpLock until m_tcpTransportTryLockTimeout to avoid blocking long
  // time, if could not get m_namesrvlock, return NULL
  LOG_DEBUG_NEW("create namesrv transport");
  if (!UtilAll::try_lock_for(m_namesrvLock, 1000 * m_tcpTransportTryLockTimeout)) {
    LOG_ERROR_NEW("CreateNameserverTransport get timed_mutex timeout");
    return nullptr;
  }
  std::lock_guard<std::timed_mutex> lock(m_namesrvLock, std::adopt_lock);

  if (!m_namesrvAddrChoosed.empty()) {
    auto channel = CreateTransport(m_namesrvAddrChoosed);
    if (channel != nullptr) {
      return channel;
    } else {
      m_namesrvAddrChoosed.clear();
    }
  }

  for (unsigned i = 0; i < m_namesrvAddrList.size(); i++) {
    auto index = m_namesrvIndex++ % m_namesrvAddrList.size();
    LOG_INFO_NEW("namesrvIndex is:{}, index:{}, namesrvaddrlist size:{}", m_namesrvIndex, index,
                 m_namesrvAddrList.size());
    auto channel = CreateTransport(m_namesrvAddrList[index]);
    if (channel != nullptr) {
      m_namesrvAddrChoosed = m_namesrvAddrList[index];
      return channel;
    }
  }

  return nullptr;
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

  LOG_INFO_NEW("CloseTransport of:{}", addr);

  bool removeItemFromTable = true;
  const auto& it = m_transportTable.find(addr);
  if (it != m_transportTable.end()) {
    if (it->second->getStartTime() != channel->getStartTime()) {
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
    LOG_ERROR_NEW("CloseNameServerTransport get timed_mutex timeout");
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
  MemoryBlockPtr package(msg.encode());
  return channel->sendMessage(package->getData(), package->getSize());
}

void TcpRemotingClient::channelClosed(TcpTransportPtr channel) {
  LOG_DEBUG_NEW("channel of {} is closed.", channel->getPeerAddrAndPort());
  m_handleExecutor.submit([channel] { static_cast<ResponseFutureInfo*>(channel->getInfo())->activeAllResponses(); });
}

void TcpRemotingClient::messageReceived(MemoryBlockPtr mem, TcpTransportPtr channel) {
  m_dispatchExecutor.submit(
      std::bind(&TcpRemotingClient::processMessageReceived, this, MemoryBlockPtr2(std::move(mem)), channel));
}

void TcpRemotingClient::processMessageReceived(MemoryBlockPtr2 mem, TcpTransportPtr channel) {
  std::unique_ptr<RemotingCommand> cmd;
  try {
    cmd.reset(RemotingCommand::Decode(mem));
  } catch (...) {
    LOG_ERROR_NEW("processMessageReceived error");
    return;
  }

  if (cmd->isResponseType()) {
    processResponseCommand(std::move(cmd), channel);
  } else {
    class task_adaptor {
     public:
      task_adaptor(TcpRemotingClient* client, std::unique_ptr<RemotingCommand> cmd, TcpTransportPtr channel)
          : client_(client), cmd_(cmd.release()), channel_(channel) {}
      task_adaptor(const task_adaptor& other) : client_(other.client_), cmd_(other.cmd_), channel_(other.channel_) {
        // force move
        const_cast<task_adaptor*>(&other)->cmd_ = nullptr;
      }
      task_adaptor(task_adaptor&& other) : client_(other.client_), cmd_(other.cmd_), channel_(other.channel_) {
        other.cmd_ = nullptr;
      }
      ~task_adaptor() { delete cmd_; }

      void operator()() {
        std::unique_ptr<RemotingCommand> requestCommand(cmd_);
        cmd_ = nullptr;
        client_->processRequestCommand(std::move(requestCommand), channel_);
      }

     private:
      TcpRemotingClient* client_;
      RemotingCommand* cmd_;
      TcpTransportPtr channel_;
    };

    m_handleExecutor.submit(task_adaptor(this, std::move(cmd), channel));
  }
}

void TcpRemotingClient::processResponseCommand(std::unique_ptr<RemotingCommand> responseCommand,
                                               TcpTransportPtr channel) {
  int opaque = responseCommand->getOpaque();
  auto responseFuture = popResponseFuture(channel, opaque);
  if (responseFuture != nullptr) {
    int code = responseFuture->getRequestCode();
    LOG_DEBUG_NEW("processResponseCommand, opaque:{}, request code:{}, server:{}", opaque, code,
                  channel->getPeerAddrAndPort());

    if (responseFuture->hasInvokeCallback()) {
      responseFuture->setResponseCommand(std::move(responseCommand));
      // bind shared_ptr can save object's life
      m_handleExecutor.submit(std::bind(&ResponseFuture::executeInvokeCallback, responseFuture));
    } else {
      responseFuture->putResponse(std::move(responseCommand));
    }
  } else {
    LOG_DEBUG_NEW("responseFuture was deleted by timeout of opaque:{}, server:{}", opaque,
                  channel->getPeerAddrAndPort());
  }
}

void TcpRemotingClient::processRequestCommand(std::unique_ptr<RemotingCommand> requestCommand,
                                              TcpTransportPtr channel) {
  std::unique_ptr<RemotingCommand> response;

  int requestCode = requestCommand->getCode();
  const auto& it = m_processorTable.find(requestCode);
  if (it != m_processorTable.end()) {
    try {
      auto* processor = it->second;

      doBeforeRpcHooks(channel->getPeerAddrAndPort(), *requestCommand, false);
      response.reset(processor->processRequest(channel, requestCommand.get()));
      doAfterRpcHooks(channel->getPeerAddrAndPort(), *response, response.get(), true);
    } catch (std::exception& e) {
      LOG_ERROR_NEW("process request exception. {}", e.what());

      // send SYSTEM_ERROR response
      response.reset(new RemotingCommand(SYSTEM_ERROR, e.what()));
    }
  } else {
    // send REQUEST_CODE_NOT_SUPPORTED response
    std::string error = "request type " + UtilAll::to_string(requestCommand->getCode()) + " not supported";
    response.reset(new RemotingCommand(REQUEST_CODE_NOT_SUPPORTED, error));

    LOG_ERROR_NEW("{}: {}", channel->getPeerAddrAndPort(), error);
  }

  if (!requestCommand->isOnewayRPC() && response != nullptr) {
    response->setOpaque(requestCommand->getOpaque());
    response->markResponseType();
    try {
      if (!SendCommand(channel, *response)) {
        LOG_WARN_NEW("send a response command to channel <{}> failed.", channel->getPeerAddrAndPort());
      }
    } catch (const std::exception& e) {
      LOG_ERROR_NEW("process request over, but response failed. {}", e.what());
    }
  }
}

void TcpRemotingClient::putResponseFuture(TcpTransportPtr channel, int opaque, ResponseFuturePtr future) {
  return static_cast<ResponseFutureInfo*>(channel->getInfo())->putResponseFuture(opaque, future);
}

// NOTE: after call this function, shared_ptr of m_futureTable[opaque] will
// be erased, so caller must ensure the life cycle of returned shared_ptr;
ResponseFuturePtr TcpRemotingClient::popResponseFuture(TcpTransportPtr channel, int opaque) {
  return static_cast<ResponseFutureInfo*>(channel->getInfo())->popResponseFuture(opaque);
}

void TcpRemotingClient::registerProcessor(MQRequestCode requestCode, RequestProcessor* requestProcessor) {
  // replace
  m_processorTable[requestCode] = requestProcessor;
}

}  // namespace rocketmq
