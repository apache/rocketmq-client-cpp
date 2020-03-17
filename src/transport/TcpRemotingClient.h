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
#ifndef __TCPREMOTINGCLIENT_H__
#define __TCPREMOTINGCLIENT_H__

#include <map>
#include <mutex>

#include <boost/asio.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "ClientRemotingProcessor.h"
#include "RemotingCommand.h"
#include "ResponseFuture.h"
#include "SocketUtil.h"
#include "TcpTransport.h"

namespace rocketmq {
//<!************************************************************************

class TcpRemotingClient {
 public:
  TcpRemotingClient(bool enableSsl, const string& sslPropertyFile);
  TcpRemotingClient(int pullThreadNum,
                    uint64_t tcpConnectTimeout,
                    uint64_t tcpTransportTryLockTimeout,
                    bool enableSsl,
                    const string& sslPropertyFile);
  virtual ~TcpRemotingClient();

  virtual void stopAllTcpTransportThread();
  virtual void updateNameServerAddressList(const string& addrs);

  virtual bool invokeHeartBeat(const string& addr, RemotingCommand& request, int timeoutMillis = 3000);

  // delete outsite;
  virtual RemotingCommand* invokeSync(const string& addr, RemotingCommand& request, int timeoutMillis = 3000);

  virtual bool invokeAsync(const string& addr,
                           RemotingCommand& request,
                           std::shared_ptr<AsyncCallbackWrap> cbw,
                           int64 timeoutMilliseconds,
                           int maxRetrySendTimes = 1,
                           int retrySendTimes = 1);

  virtual void invokeOneway(const string& addr, RemotingCommand& request);

  virtual void registerProcessor(MQRequestCode requestCode, ClientRemotingProcessor* clientRemotingProcessor);

 private:
  static void static_messageReceived(void* context, const MemoryBlock& mem, const string& addr);

  void messageReceived(const MemoryBlock& mem, const string& addr);
  void ProcessData(const MemoryBlock& mem, const string& addr);
  void processRequestCommand(RemotingCommand* pCmd, const string& addr);
  void processResponseCommand(RemotingCommand* pCmd, std::shared_ptr<ResponseFuture> pFuture);
  void handleAsyncRequestTimeout(const boost::system::error_code& e, int opaque);

  std::shared_ptr<TcpTransport> GetTransport(const string& addr, bool needResponse);
  std::shared_ptr<TcpTransport> CreateTransport(const string& addr, bool needResponse);
  std::shared_ptr<TcpTransport> CreateNameServerTransport(bool needResponse);

  bool CloseTransport(const string& addr, std::shared_ptr<TcpTransport> pTcp);
  bool CloseNameServerTransport(std::shared_ptr<TcpTransport> pTcp);

  bool SendCommand(std::shared_ptr<TcpTransport> pTts, RemotingCommand& msg);

  void addResponseFuture(int opaque, std::shared_ptr<ResponseFuture> pFuture);
  std::shared_ptr<ResponseFuture> findAndDeleteResponseFuture(int opaque);

  void addTimerCallback(boost::asio::deadline_timer* t, int opaque);
  void eraseTimerCallback(int opaque);
  void cancelTimerCallback(int opaque);
  void removeAllTimerCallback();

  void boost_asio_work();

 private:
  using RequestMap = map<int, ClientRemotingProcessor*>;
  using TcpMap = map<string, std::shared_ptr<TcpTransport>>;
  using ResMap = map<int, std::shared_ptr<ResponseFuture>>;
  using AsyncTimerMap = map<int, boost::asio::deadline_timer*>;

  RequestMap m_requestTable;

  TcpMap m_tcpTable;  //<! addr->tcp;
  std::timed_mutex m_tcpTableLock;

  ResMap m_futureTable;  //<! id->future;
  std::mutex m_futureTableLock;

  AsyncTimerMap m_asyncTimerTable;
  std::mutex m_asyncTimerTableLock;

  int m_dispatchThreadNum;
  int m_pullThreadNum;
  uint64_t m_tcpConnectTimeout;           // ms
  uint64_t m_tcpTransportTryLockTimeout;  // s

  bool m_enableSsl;
  std::string m_sslPropertyFile;

  //<! NameServer
  std::timed_mutex m_namesrvLock;
  vector<string> m_namesrvAddrList;
  string m_namesrvAddrChoosed;
  unsigned int m_namesrvIndex;

  boost::asio::io_service m_dispatchService;
  boost::asio::io_service::work m_dispatchServiceWork;
  boost::thread_group m_dispatchThreadPool;

  boost::asio::io_service m_handleService;
  boost::asio::io_service::work m_handleServiceWork;
  boost::thread_group m_handleThreadPool;

  boost::asio::io_service m_timerService;
  unique_ptr<boost::thread> m_timerServiceThread;
};

//<!************************************************************************
}  // namespace rocketmq

#endif
