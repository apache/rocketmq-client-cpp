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
#ifndef __TCP_TRANSPORT_H__
#define __TCP_TRANSPORT_H__

#include <atomic>
#include <condition_variable>
#include <mutex>

#include "ByteArray.h"
#include "EventLoop.h"

namespace rocketmq {

typedef enum TcpConnectStatus {
  TCP_CONNECT_STATUS_CREATED = 0,
  TCP_CONNECT_STATUS_CONNECTING = 1,
  TCP_CONNECT_STATUS_CONNECTED = 2,
  TCP_CONNECT_STATUS_FAILED = 3,
  TCP_CONNECT_STATUS_CLOSED = 4
} TcpConnectStatus;

class TcpTransport;
typedef std::shared_ptr<TcpTransport> TcpTransportPtr;

class TcpTransportInfo {
 public:
  virtual ~TcpTransportInfo() = default;
};

class TcpTransport : public noncopyable, public std::enable_shared_from_this<TcpTransport> {
 public:
  typedef std::function<void(ByteArrayRef, TcpTransportPtr) noexcept> ReadCallback;
  typedef std::function<void(TcpTransportPtr) noexcept> CloseCallback;

 public:
  static TcpTransportPtr CreateTransport(ReadCallback readCallback,
                                         CloseCallback closeCallback,
                                         std::unique_ptr<TcpTransportInfo> info) {
    // transport must be managed by smart pointer
    return TcpTransportPtr(new TcpTransport(readCallback, closeCallback, std::move(info)));
  }

  virtual ~TcpTransport();

  void disconnect(const std::string& addr);
  TcpConnectStatus connect(const std::string& strServerURL, int timeoutMillis = 3000);
  TcpConnectStatus waitTcpConnectEvent(int timeoutMillis = 3000);
  TcpConnectStatus getTcpConnectStatus();

  bool sendMessage(const char* pData, size_t len);
  const std::string& getPeerAddrAndPort();
  const uint64_t getStartTime() const;

  TcpTransportInfo* getInfo() { return m_info.get(); }

 private:
  // don't instance object directly.
  TcpTransport(ReadCallback readCallback, CloseCallback closeCallback, std::unique_ptr<TcpTransportInfo> info);

  // BufferEvent callback
  void dataArrived(BufferEvent& event);
  void eventOccurred(BufferEvent& event, short what);

  void messageReceived(ByteArrayRef msg);

  TcpConnectStatus closeBufferEvent(bool isDeleted = false);

  TcpConnectStatus setTcpConnectEvent(TcpConnectStatus connectStatus);
  bool setTcpConnectEventIf(TcpConnectStatus& expectStatus, TcpConnectStatus connectStatus);

 private:
  uint64_t m_startTime;

  std::unique_ptr<BufferEvent> m_event;  // NOTE: use m_event in callback is unsafe.

  std::atomic<TcpConnectStatus> m_tcpConnectStatus;
  std::mutex m_statusMutex;
  std::condition_variable m_statusEvent;

  // callback
  ReadCallback m_readCallback;
  CloseCallback m_closeCallback;

  // info
  std::unique_ptr<TcpTransportInfo> m_info;
};

}  // namespace rocketmq

#endif  // __TCP_TRANSPORT__
