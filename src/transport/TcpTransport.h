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
#ifndef __TCPTRANSPORT_H__
#define __TCPTRANSPORT_H__

#include <atomic>
#include <condition_variable>
#include <mutex>

#include "EventLoop.h"
#include "dataBlock.h"

namespace rocketmq {

//<!***************************************************************************
typedef enum TcpConnectStatus {
  TCP_CONNECT_STATUS_INIT = 0,
  TCP_CONNECT_STATUS_WAIT = 1,
  TCP_CONNECT_STATUS_SUCCESS = 2,
  TCP_CONNECT_STATUS_FAILED = 3
} TcpConnectStatus;

using TcpTransportReadCallback = void (*)(void* context, const MemoryBlock&, const std::string&);

class TcpRemotingClient;

class TcpTransport : public std::enable_shared_from_this<TcpTransport> {
 public:
  static std::shared_ptr<TcpTransport> CreateTransport(TcpRemotingClient* pTcpRemotingClient,
                                                       bool enableSsl,
                                                       const std::string& sslPropertyFile,
                                                       TcpTransportReadCallback handle = nullptr) {
    // transport must be managed by smart pointer
    std::shared_ptr<TcpTransport> transport(new TcpTransport(pTcpRemotingClient, enableSsl, sslPropertyFile, handle));
    return transport;
  }

  virtual ~TcpTransport();

  void disconnect(const std::string& addr);
  TcpConnectStatus connect(const std::string& strServerURL, int timeoutMillis = 3000);
  TcpConnectStatus waitTcpConnectEvent(int timeoutMillis = 3000);
  TcpConnectStatus getTcpConnectStatus();

  bool sendMessage(const char* pData, size_t len);
  const std::string getPeerAddrAndPort();
  const uint64_t getStartTime() const;

 private:
  TcpTransport(TcpRemotingClient* pTcpRemotingClient,
               bool enableSsl,
               const std::string& sslPropertyFile,
               TcpTransportReadCallback handle = nullptr);

  static void readNextMessageIntCallback(BufferEvent* event, TcpTransport* transport);
  static void eventCallback(BufferEvent* event, short what, TcpTransport* transport);

  void messageReceived(const MemoryBlock& mem, const std::string& addr);
  void freeBufferEvent();  // not thread-safe

  void setTcpConnectEvent(TcpConnectStatus connectStatus);
  void setTcpConnectStatus(TcpConnectStatus connectStatus);

  u_long getInetAddr(std::string& hostname);

 private:
  uint64_t m_startTime;

  std::shared_ptr<BufferEvent> m_event;  // NOTE: use m_event in callback is unsafe.
  std::mutex m_eventLock;
  std::atomic<TcpConnectStatus> m_tcpConnectStatus;

  std::mutex m_connectEventLock;
  std::condition_variable m_connectEvent;

  //<! read data callback
  TcpTransportReadCallback m_readCallback;
  TcpRemotingClient* m_tcpRemotingClient;

  bool m_enableSsl;
  std::string m_sslPropertyFile;
};

//<!************************************************************************
}  // namespace rocketmq

#endif
