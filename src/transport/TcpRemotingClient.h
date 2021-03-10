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
#ifndef ROCKETMQ_TRANSPORT_TCPREMOTINGCLIENT_H_
#define ROCKETMQ_TRANSPORT_TCPREMOTINGCLIENT_H_

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "MQException.h"
#include "MQProtos.h"
#include "RPCHook.h"
#include "RemotingCommand.h"
#include "RequestProcessor.h"
#include "ResponseFuture.h"
#include "TcpTransport.h"
#include "concurrent/executor.hpp"

namespace rocketmq {

class TcpRemotingClient {
 public:
  TcpRemotingClient(int workerThreadNum, uint64_t tcpConnectTimeout, uint64_t tcpTransportTryLockTimeout);
  virtual ~TcpRemotingClient();

  void start();
  void shutdown();

  void registerRPCHook(RPCHookPtr rpcHook);

  void updateNameServerAddressList(const std::string& addrs);

  std::unique_ptr<RemotingCommand> invokeSync(const std::string& addr,
                                              RemotingCommand& request,
                                              int timeoutMillis = 3000);

  void invokeAsync(const std::string& addr,
                   RemotingCommand& request,
                   std::unique_ptr<InvokeCallback>& invokeCallback,
                   int64_t timeoutMillis);

  void invokeOneway(const std::string& addr, RemotingCommand& request);

  void registerProcessor(MQRequestCode requestCode, RequestProcessor* requestProcessor);

  std::vector<std::string> getNameServerAddressList() const { return namesrv_addr_list_; }

 private:
  static bool SendCommand(TcpTransportPtr channel, RemotingCommand& msg) noexcept;

  void channelClosed(TcpTransportPtr channel);

  void messageReceived(ByteArrayRef msg, TcpTransportPtr channel);
  void processMessageReceived(ByteArrayRef msg, TcpTransportPtr channel);
  void processRequestCommand(std::unique_ptr<RemotingCommand> cmd, TcpTransportPtr channel);
  void processResponseCommand(std::unique_ptr<RemotingCommand> cmd, TcpTransportPtr channel);

  // timeout daemon
  void scanResponseTablePeriodically();
  void scanResponseTable();

  TcpTransportPtr GetTransport(const std::string& addr);
  TcpTransportPtr CreateTransport(const std::string& addr);
  TcpTransportPtr CreateNameServerTransport();

  bool CloseTransport(const std::string& addr, TcpTransportPtr channel);
  bool CloseNameServerTransport(TcpTransportPtr channel);

  std::unique_ptr<RemotingCommand> invokeSyncImpl(TcpTransportPtr channel,
                                                  RemotingCommand& request,
                                                  int64_t timeoutMillis);
  void invokeAsyncImpl(TcpTransportPtr channel,
                       RemotingCommand& request,
                       int64_t timeoutMillis,
                       std::unique_ptr<InvokeCallback>& invokeCallback);
  void invokeOnewayImpl(TcpTransportPtr channel, RemotingCommand& request);

  // rpc hook
  void doBeforeRpcHooks(const std::string& addr, RemotingCommand& request, bool toSent);
  void doAfterRpcHooks(const std::string& addr, RemotingCommand& request, RemotingCommand* response, bool toSent);

  // future management
  void putResponseFuture(TcpTransportPtr channel, int opaque, ResponseFuturePtr future);
  ResponseFuturePtr popResponseFuture(TcpTransportPtr channel, int opaque);

 private:
  using ProcessorMap = std::map<int, RequestProcessor*>;
  using TransportMap = std::map<std::string, std::shared_ptr<TcpTransport>>;

  ProcessorMap processor_table_;  // code -> processor

  TransportMap transport_table_;  // addr -> transport
  std::timed_mutex transport_table_mutex_;

  // FIXME: not strict thread-safe in abnormal scence
  std::vector<RPCHookPtr> rpc_hooks_;  // for Acl / ONS

  uint64_t tcp_connect_timeout_;             // ms
  uint64_t tcp_transport_try_lock_timeout_;  // s

  // NameServer
  std::timed_mutex namesrv_lock_;
  std::vector<std::string> namesrv_addr_list_;
  std::string namesrv_addr_choosed_;
  size_t namesrv_index_;

  thread_pool_executor dispatch_executor_;
  thread_pool_executor handle_executor_;
  scheduled_thread_pool_executor timeout_executor_;
};

}  // namespace rocketmq

#endif  // ROCKETMQ_TRANSPORT_TCPREMOTINGCLIENT_H_
