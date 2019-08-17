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
#include <mutex>

#include "concurrent/executor.hpp"

#include "ClientRemotingProcessor.h"
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
  void updateNameServerAddressList(const std::string& addrs);

  bool invokeHeartBeat(const std::string& addr, RemotingCommand& request, int timeoutMillis = 3000);

  // delete outsite
  RemotingCommand* invokeSync(const std::string& addr, RemotingCommand& request, int timeoutMillis = 3000);

  bool invokeAsync(const std::string& addr,
                   RemotingCommand& request,
                   AsyncCallbackWrap* cbw,
                   int64 timeoutMillis,
                   int maxRetrySendTimes = 1,
                   int retrySendTimes = 1);

  void invokeOneway(const std::string& addr, RemotingCommand& request);

  void registerProcessor(MQRequestCode requestCode, ClientRemotingProcessor* clientRemotingProcessor);

 private:
  static void static_messageReceived(void* context, const MemoryBlock& mem, const std::string& addr);

  void messageReceived(const MemoryBlock& mem, const std::string& addr);
  void ProcessData(const MemoryBlock& mem, const std::string& addr);
  void processRequestCommand(RemotingCommand* pCmd, const std::string& addr);
  void processResponseCommand(RemotingCommand* pCmd, std::shared_ptr<ResponseFuture> pFuture);
  void checkAsyncRequestTimeout(int opaque);

  std::shared_ptr<TcpTransport> GetTransport(const std::string& addr, bool needResponse);
  std::shared_ptr<TcpTransport> CreateTransport(const std::string& addr, bool needResponse);
  std::shared_ptr<TcpTransport> CreateNameServerTransport(bool needResponse);

  bool CloseTransport(const std::string& addr, std::shared_ptr<TcpTransport> pTcp);
  bool CloseNameServerTransport(std::shared_ptr<TcpTransport> pTcp);

  bool SendCommand(std::shared_ptr<TcpTransport> pTts, RemotingCommand& msg);

  void addResponseFuture(int opaque, std::shared_ptr<ResponseFuture> pFuture);
  std::shared_ptr<ResponseFuture> findAndDeleteResponseFuture(int opaque);

 private:
  using RequestMap = std::map<int, ClientRemotingProcessor*>;
  using TcpMap = std::map<std::string, std::shared_ptr<TcpTransport>>;
  using ResMap = std::map<int, std::shared_ptr<ResponseFuture>>;

  RequestMap m_requestTable;

  TcpMap m_tcpTable;  // addr->tcp;
  std::timed_mutex m_tcpTableLock;

  ResMap m_futureTable;  // id->future;
  std::mutex m_futureTableLock;

  int m_dispatchThreadNum;
  int m_pullThreadNum;
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
