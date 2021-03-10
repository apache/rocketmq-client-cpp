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

#include <cstddef>

#include <algorithm>  // std::move

#include "Logging.h"
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
    std::lock_guard<std::mutex> lock(future_table_mutex_);
    const auto it = future_table_.find(opaque);
    if (it != future_table_.end()) {
      LOG_WARN_NEW("[BUG] response futurn with opaque:{} is exist.", opaque);
    }
    future_table_[opaque] = future;
  }

  ResponseFuturePtr popResponseFuture(int opaque) {
    std::lock_guard<std::mutex> lock(future_table_mutex_);
    const auto& it = future_table_.find(opaque);
    if (it != future_table_.end()) {
      auto future = it->second;
      future_table_.erase(it);
      return future;
    }
    return nullptr;
  }

  void scanResponseTable(uint64_t now, std::vector<ResponseFuturePtr>& futureList) {
    std::lock_guard<std::mutex> lock(future_table_mutex_);
    for (auto it = future_table_.begin(); it != future_table_.end();) {
      auto& future = it->second;  // NOTE: future is a reference
      if (future->begin_timestamp() + future->timeout_millis() + 1000 <= now) {
        LOG_WARN_NEW("remove timeout request, code:{}, opaque:{}", future->request_code(), future->opaque());
        futureList.push_back(future);
        it = future_table_.erase(it);
      } else {
        ++it;
      }
    }
  }

  void activeAllResponses() {
    std::lock_guard<std::mutex> lock(future_table_mutex_);
    for (const auto& it : future_table_) {
      auto& future = it.second;
      if (future->hasInvokeCallback()) {
        future->executeInvokeCallback();  // callback async request
      } else {
        future->putResponse(nullptr);  // wake up sync request
      }
    }
    future_table_.clear();
  }

 private:
  FutureMap future_table_;  // opaque -> future
  std::mutex future_table_mutex_;
};

TcpRemotingClient::TcpRemotingClient(int workerThreadNum,
                                     uint64_t tcpConnectTimeout,
                                     uint64_t tcpTransportTryLockTimeout)
    : tcp_connect_timeout_(tcpConnectTimeout),
      tcp_transport_try_lock_timeout_(tcpTransportTryLockTimeout),
      namesrv_index_(0),
      dispatch_executor_("MessageDispatchExecutor", 1, false),
      handle_executor_("MessageHandleExecutor", workerThreadNum, false),
      timeout_executor_("TimeoutScanExecutor", false) {}

TcpRemotingClient::~TcpRemotingClient() {
  LOG_DEBUG_NEW("TcpRemotingClient is destructed.");
}

void TcpRemotingClient::start() {
  dispatch_executor_.startup();
  handle_executor_.startup();

  LOG_INFO_NEW("tcpConnectTimeout:{}, tcpTransportTryLockTimeout:{}, pullThreadNums:{}", tcp_connect_timeout_,
               tcp_transport_try_lock_timeout_, handle_executor_.thread_nums());

  timeout_executor_.startup();

  // scanResponseTable
  timeout_executor_.schedule(std::bind(&TcpRemotingClient::scanResponseTablePeriodically, this), 1000 * 3,
                             time_unit::milliseconds);
}

void TcpRemotingClient::shutdown() {
  LOG_INFO_NEW("TcpRemotingClient::shutdown Begin");

  timeout_executor_.shutdown();

  {
    std::lock_guard<std::timed_mutex> lock(transport_table_mutex_);
    for (const auto& trans : transport_table_) {
      trans.second->disconnect(trans.first);
    }
    transport_table_.clear();
  }

  handle_executor_.shutdown();

  dispatch_executor_.shutdown();

  LOG_INFO_NEW("TcpRemotingClient::shutdown End, transport_table_:{}", transport_table_.size());
}

void TcpRemotingClient::registerRPCHook(RPCHookPtr rpcHook) {
  if (rpcHook != nullptr) {
    for (auto& hook : rpc_hooks_) {
      if (hook == rpcHook) {
        return;
      }
    }

    rpc_hooks_.push_back(rpcHook);
  }
}

void TcpRemotingClient::updateNameServerAddressList(const std::string& addrs) {
  LOG_INFO_NEW("updateNameServerAddressList: [{}]", addrs);

  if (addrs.empty()) {
    return;
  }

  if (!UtilAll::try_lock_for(namesrv_lock_, 1000 * 10)) {
    LOG_ERROR_NEW("updateNameServerAddressList get timed_mutex timeout");
    return;
  }
  std::lock_guard<std::timed_mutex> lock(namesrv_lock_, std::adopt_lock);

  // clear first
  namesrv_addr_list_.clear();

  std::vector<std::string> out;
  UtilAll::Split(out, addrs, ";");
  for (auto& addr : out) {
    UtilAll::Trim(addr);

    std::string hostName;
    short portNumber;
    if (UtilAll::SplitURL(addr, hostName, portNumber)) {
      LOG_INFO_NEW("update Namesrv:{}", addr);
      namesrv_addr_list_.push_back(addr);
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
  timeout_executor_.schedule(std::bind(&TcpRemotingClient::scanResponseTablePeriodically, this), 1000,
                             time_unit::milliseconds);
}

void TcpRemotingClient::scanResponseTable() {
  std::vector<TcpTransportPtr> channelList;
  {
    std::lock_guard<std::timed_mutex> lock(transport_table_mutex_);
    for (const auto& trans : transport_table_) {
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
      handle_executor_.submit(std::bind(&ResponseFuture::executeInvokeCallback, rf));
    }
  }
}

std::unique_ptr<RemotingCommand> TcpRemotingClient::invokeSync(const std::string& addr,
                                                               RemotingCommand& request,
                                                               int timeoutMillis) {
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
      throw;
    } catch (const RemotingTimeoutException& e) {
      int code = request.code();
      if (code != GET_CONSUMER_LIST_BY_GROUP) {
        CloseTransport(addr, channel);
        LOG_WARN_NEW("invokeSync: close socket because of timeout, {}ms, {}", timeoutMillis,
                     channel->getPeerAddrAndPort());
      }
      LOG_WARN_NEW("invokeSync: wait response timeout exception, the channel[{}]", channel->getPeerAddrAndPort());
      throw;
    }
  } else {
    THROW_MQEXCEPTION(RemotingConnectException, "connect to <" + addr + "> failed", -1);
  }
}

std::unique_ptr<RemotingCommand> TcpRemotingClient::invokeSyncImpl(TcpTransportPtr channel,
                                                                   RemotingCommand& request,
                                                                   int64_t timeoutMillis) {
  int code = request.code();
  int opaque = request.opaque();

  auto responseFuture = std::make_shared<ResponseFuture>(code, opaque, timeoutMillis);
  putResponseFuture(channel, opaque, responseFuture);

  if (SendCommand(channel, request)) {
    responseFuture->set_send_request_ok(true);
  } else {
    responseFuture->set_send_request_ok(false);
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
                                    std::unique_ptr<InvokeCallback>& invokeCallback,
                                    int64_t timeoutMillis) {
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
      throw;
    }
  } else {
    THROW_MQEXCEPTION(RemotingConnectException, "connect to <" + addr + "> failed", -1);
  }
}

void TcpRemotingClient::invokeAsyncImpl(TcpTransportPtr channel,
                                        RemotingCommand& request,
                                        int64_t timeoutMillis,
                                        std::unique_ptr<InvokeCallback>& invokeCallback) {
  int code = request.code();
  int opaque = request.opaque();

  // delete in callback
  auto responseFuture = std::make_shared<ResponseFuture>(code, opaque, timeoutMillis, std::move(invokeCallback));
  putResponseFuture(channel, opaque, responseFuture);

  if (SendCommand(channel, request)) {
    responseFuture->set_send_request_ok(true);
  } else {
    // request fail
    responseFuture = popResponseFuture(channel, opaque);
    if (responseFuture != nullptr) {
      responseFuture->set_send_request_ok(false);
      if (responseFuture->hasInvokeCallback()) {
        handle_executor_.submit(std::bind(&ResponseFuture::executeInvokeCallback, responseFuture));
      }
    }

    LOG_WARN_NEW("send a request command to channel <{}> failed.", channel->getPeerAddrAndPort());
  }
}

void TcpRemotingClient::invokeOneway(const std::string& addr, RemotingCommand& request) {
  auto channel = GetTransport(addr);
  if (channel != nullptr) {
    try {
      doBeforeRpcHooks(addr, request, true);
      invokeOnewayImpl(channel, request);
    } catch (const RemotingSendRequestException& e) {
      LOG_WARN_NEW("invokeOneway: send request exception, so close the channel[{}]", channel->getPeerAddrAndPort());
      CloseTransport(addr, channel);
      throw;
    }
  } else {
    THROW_MQEXCEPTION(RemotingConnectException, "connect to <" + addr + "> failed", -1);
  }
}

void TcpRemotingClient::invokeOnewayImpl(TcpTransportPtr channel, RemotingCommand& request) {
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
  if (rpc_hooks_.size() > 0) {
    for (auto& rpcHook : rpc_hooks_) {
      rpcHook->doBeforeRequest(addr, request, toSent);
    }
  }
}

void TcpRemotingClient::doAfterRpcHooks(const std::string& addr,
                                        RemotingCommand& request,
                                        RemotingCommand* response,
                                        bool toSent) {
  if (rpc_hooks_.size() > 0) {
    for (auto& rpcHook : rpc_hooks_) {
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
    // try get transport_table_mutex_ until tcp_transport_try_lock_timeout_ to avoid blocking long time,
    // if could not get transport_table_mutex_, return NULL
    if (!UtilAll::try_lock_for(transport_table_mutex_, 1000 * tcp_transport_try_lock_timeout_)) {
      LOG_ERROR_NEW("GetTransport of:{} get timed_mutex timeout", addr);
      return nullptr;
    }
    std::lock_guard<std::timed_mutex> lock(transport_table_mutex_, std::adopt_lock);

    // check for reuse
    const auto& it = transport_table_.find(addr);
    if (it != transport_table_.end()) {
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
          transport_table_.erase(it);
          break;
        default:  // TCP_CONNECT_STATUS_CLOSED
          LOG_ERROR_NEW("go to CLOSED state, erase:{} from transportTable, and reconnect it", addr);
          transport_table_.erase(it);
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
        transport_table_[addr] = channel;
      }
    }
  }

  // waiting...
  TcpConnectStatus connectStatus = channel->waitTcpConnectEvent(static_cast<int>(tcp_connect_timeout_));
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
  // namesrv_lock_ was added to avoid operation of NameServer was blocked by
  // m_tcpLock, it was used by single Thread mostly, so no performance impact
  // try get m_tcpLock until tcp_transport_try_lock_timeout_ to avoid blocking long
  // time, if could not get m_namesrvlock, return NULL
  LOG_DEBUG_NEW("create namesrv transport");
  if (!UtilAll::try_lock_for(namesrv_lock_, 1000 * tcp_transport_try_lock_timeout_)) {
    LOG_ERROR_NEW("CreateNameserverTransport get timed_mutex timeout");
    return nullptr;
  }
  std::lock_guard<std::timed_mutex> lock(namesrv_lock_, std::adopt_lock);

  if (!namesrv_addr_choosed_.empty()) {
    auto channel = CreateTransport(namesrv_addr_choosed_);
    if (channel != nullptr) {
      return channel;
    } else {
      namesrv_addr_choosed_.clear();
    }
  }

  for (unsigned i = 0; i < namesrv_addr_list_.size(); i++) {
    auto index = namesrv_index_++ % namesrv_addr_list_.size();
    LOG_INFO_NEW("namesrvIndex is:{}, index:{}, namesrvaddrlist size:{}", namesrv_index_, index,
                 namesrv_addr_list_.size());
    auto channel = CreateTransport(namesrv_addr_list_[index]);
    if (channel != nullptr) {
      namesrv_addr_choosed_ = namesrv_addr_list_[index];
      return channel;
    }
  }

  return nullptr;
}

bool TcpRemotingClient::CloseTransport(const std::string& addr, TcpTransportPtr channel) {
  if (addr.empty()) {
    return CloseNameServerTransport(channel);
  }

  if (!UtilAll::try_lock_for(transport_table_mutex_, 1000 * tcp_transport_try_lock_timeout_)) {
    LOG_ERROR_NEW("CloseTransport of:{} get timed_mutex timeout", addr);
    return false;
  }
  std::lock_guard<std::timed_mutex> lock(transport_table_mutex_, std::adopt_lock);

  LOG_INFO_NEW("CloseTransport of:{}", addr);

  bool removeItemFromTable = true;
  const auto& it = transport_table_.find(addr);
  if (it != transport_table_.end()) {
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
    transport_table_.erase(addr);
  }

  LOG_WARN_NEW("closeTransport: disconnect:{} with state:{}", addr, channel->getTcpConnectStatus());
  if (channel->getTcpConnectStatus() != TCP_CONNECT_STATUS_CLOSED) {
    channel->disconnect(addr);  // avoid coredump when connection with server was broken
  }

  LOG_ERROR_NEW("CloseTransport of:{} end", addr);

  return removeItemFromTable;
}

bool TcpRemotingClient::CloseNameServerTransport(TcpTransportPtr channel) {
  if (!UtilAll::try_lock_for(namesrv_lock_, 1000 * tcp_transport_try_lock_timeout_)) {
    LOG_ERROR_NEW("CloseNameServerTransport get timed_mutex timeout");
    return false;
  }
  std::lock_guard<std::timed_mutex> lock(namesrv_lock_, std::adopt_lock);

  std::string addr = namesrv_addr_choosed_;

  bool removeItemFromTable = CloseTransport(addr, channel);
  if (removeItemFromTable) {
    namesrv_addr_choosed_.clear();
  }

  return removeItemFromTable;
}

bool TcpRemotingClient::SendCommand(TcpTransportPtr channel, RemotingCommand& msg) noexcept {
  auto package = msg.encode();
  return channel->sendMessage(package->array(), package->size());
}

void TcpRemotingClient::channelClosed(TcpTransportPtr channel) {
  LOG_DEBUG_NEW("channel of {} is closed.", channel->getPeerAddrAndPort());
  handle_executor_.submit([channel] { static_cast<ResponseFutureInfo*>(channel->getInfo())->activeAllResponses(); });
}

void TcpRemotingClient::messageReceived(ByteArrayRef msg, TcpTransportPtr channel) {
  dispatch_executor_.submit(std::bind(&TcpRemotingClient::processMessageReceived, this, std::move(msg), channel));
}

void TcpRemotingClient::processMessageReceived(ByteArrayRef msg, TcpTransportPtr channel) {
  std::unique_ptr<RemotingCommand> cmd;
  try {
    cmd.reset(RemotingCommand::Decode(std::move(msg)));
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

    handle_executor_.submit(task_adaptor(this, std::move(cmd), channel));
  }
}

void TcpRemotingClient::processResponseCommand(std::unique_ptr<RemotingCommand> responseCommand,
                                               TcpTransportPtr channel) {
  int opaque = responseCommand->opaque();
  auto responseFuture = popResponseFuture(channel, opaque);
  if (responseFuture != nullptr) {
    int code = responseFuture->request_code();
    LOG_DEBUG_NEW("processResponseCommand, opaque:{}, request code:{}, server:{}", opaque, code,
                  channel->getPeerAddrAndPort());

    if (responseFuture->hasInvokeCallback()) {
      responseFuture->setResponseCommand(std::move(responseCommand));
      // bind shared_ptr can save object's life
      handle_executor_.submit(std::bind(&ResponseFuture::executeInvokeCallback, responseFuture));
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

  int requestCode = requestCommand->code();
  const auto& it = processor_table_.find(requestCode);
  if (it != processor_table_.end()) {
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
    std::string error = "request type " + UtilAll::to_string(requestCommand->code()) + " not supported";
    response.reset(new RemotingCommand(REQUEST_CODE_NOT_SUPPORTED, error));

    LOG_ERROR_NEW("{}: {}", channel->getPeerAddrAndPort(), error);
  }

  if (!requestCommand->isOnewayRPC() && response != nullptr) {
    response->set_opaque(requestCommand->opaque());
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

// NOTE: after call this function, shared_ptr of future_table_[opaque] will
// be erased, so caller must ensure the life cycle of returned shared_ptr
ResponseFuturePtr TcpRemotingClient::popResponseFuture(TcpTransportPtr channel, int opaque) {
  return static_cast<ResponseFutureInfo*>(channel->getInfo())->popResponseFuture(opaque);
}

void TcpRemotingClient::registerProcessor(MQRequestCode requestCode, RequestProcessor* requestProcessor) {
  // replace
  processor_table_[requestCode] = requestProcessor;
}

}  // namespace rocketmq
