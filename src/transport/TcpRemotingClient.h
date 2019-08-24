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
#ifndef __TCP_REMOTING_CLIENT_H__
#define __TCP_REMOTING_CLIENT_H__

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "concurrent/executor.hpp"

#include "ClientRemotingProcessor.h"
#include "RPCHook.h"
#include "RemotingCommand.h"
#include "ResponseFuture.h"
#include "SocketUtil.h"
#include "TcpTransport.h"

namespace rocketmq {

class TcpRemotingClient {
 public:
  TcpRemotingClient(int pullThreadNum, uint64_t tcpConnectTimeout, uint64_t tcpTransportTryLockTimeout);
  virtual ~TcpRemotingClient();

  void stopAllTcpTransportThread();

  void registerRPCHook(std::shared_ptr<RPCHook> rpcHook);

  void updateNameServerAddressList(const std::string& addrs);

  bool invokeHeartBeat(const std::string& addr, RemotingCommand& request, int timeoutMillis = 3000);

  // delete outsite
  RemotingCommand* invokeSync(const std::string& addr, RemotingCommand& request, int timeoutMillis = 3000);

  bool invokeAsync(const std::string& addr,
                   RemotingCommand& request,
                   InvokeCallback* invokeCallback,
                   int64 timeoutMillis);

  void invokeOneway(const std::string& addr, RemotingCommand& request);

  void registerProcessor(MQRequestCode requestCode, ClientRemotingProcessor* clientRemotingProcessor);

 private:
  static bool SendCommand(std::shared_ptr<TcpTransport> pTts, RemotingCommand& msg);
  static void MessageReceived(void* context, const MemoryBlock& mem, const std::string& addr);

  void messageReceived(const MemoryBlock& mem, const std::string& addr);
  void processMessageReceived(const MemoryBlock& mem, const std::string& addr);
  void processRequestCommand(RemotingCommand* cmd, const std::string& addr);
  void processResponseCommand(RemotingCommand* cmd);

  void checkAsyncRequestTimeout(int opaque);

  std::shared_ptr<TcpTransport> GetTransport(const std::string& addr, bool needResponse);
  std::shared_ptr<TcpTransport> CreateTransport(const std::string& addr, bool needResponse);
  std::shared_ptr<TcpTransport> CreateNameServerTransport(bool needResponse);

  bool CloseTransport(const std::string& addr, std::shared_ptr<TcpTransport> pTcp);
  bool CloseNameServerTransport(std::shared_ptr<TcpTransport> pTcp);

  RemotingCommand* invokeSyncImpl(std::shared_ptr<TcpTransport> pTcp,
                                  RemotingCommand& request,
                                  int64 timeoutMillis) throw(RemotingTimeoutException, RemotingSendRequestException);
  void invokeAsyncImpl(std::shared_ptr<TcpTransport> pTcp,
                       RemotingCommand& request,
                       int64 timeoutMillis,
                       InvokeCallback* invokeCallback) throw(RemotingSendRequestException);
  void invokeOnewayImpl(std::shared_ptr<TcpTransport> pTcp, RemotingCommand& request);

  // rpc hook
  void doBeforeRpcHooks(const std::string& addr, RemotingCommand& request, bool toSent);
  void doAfterRpcHooks(const std::string& addr, RemotingCommand& request, RemotingCommand* response, bool toSent);

  void addResponseFuture(int opaque, std::shared_ptr<ResponseFuture> pFuture);
  std::shared_ptr<ResponseFuture> findAndDeleteResponseFuture(int opaque);

 private:
  using ProcessorMap = std::map<int, ClientRemotingProcessor*>;
  using TransportMap = std::map<std::string, std::shared_ptr<TcpTransport>>;
  using FutureMap = std::map<int, std::shared_ptr<ResponseFuture>>;

  ProcessorMap m_processorTable;  // code -> processor

  TransportMap m_transportTable;  // addr -> transport
  std::timed_mutex m_transportTableMutex;

  FutureMap m_futureTable;  // opaque -> future
  std::mutex m_futureTableMutex;

  // FIXME: not strict thread-safe in abnormal scence
  std::vector<std::shared_ptr<RPCHook>> m_rpcHooks;  // for Acl / ONS

  uint64_t m_tcpConnectTimeout;           // ms
  uint64_t m_tcpTransportTryLockTimeout;  // s

  // NameServer
  std::timed_mutex m_namesrvLock;
  std::vector<std::string> m_namesrvAddrList;
  std::string m_namesrvAddrChoosed;
  unsigned int m_namesrvIndex;

  thread_pool_executor m_dispatchExecutor;
  thread_pool_executor m_handleExecutor;
  scheduled_thread_pool_executor m_timeoutExecutor;
};

}  // namespace rocketmq

#endif  // __TCP_REMOTING_CLIENT_H__
