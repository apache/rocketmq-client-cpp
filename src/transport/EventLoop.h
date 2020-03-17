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
#ifndef __EVENTLOOP_H__
#define __EVENTLOOP_H__

#include <memory>
#include <thread>

#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/bufferevent_ssl.h>
#include <event2/event.h>
#include <openssl/err.h>
#include <openssl/ssl.h>

#include "UtilAll.h"
#include "noncopyable.h"

using socket_t = evutil_socket_t;

namespace rocketmq {

class BufferEvent;

class EventLoop : public noncopyable {
 public:
  static EventLoop* GetDefaultEventLoop();

 public:
  explicit EventLoop(const struct event_config* config = nullptr, bool run_immediately = true);
  virtual ~EventLoop();

  void start();
  void stop();

  BufferEvent* createBufferEvent(socket_t fd, int options, bool enable_ssl, const std::string& ssl_property_file);

 private:
  void runLoop();
  bool CreateSslContext(const std::string& ssl_property_file);

 private:
  struct event_base* m_eventBase{nullptr};
  std::thread* m_loopThread{nullptr};
  using SSL_CTX_ptr = std::unique_ptr<SSL_CTX, decltype(::SSL_CTX_free)&>;
  SSL_CTX_ptr m_sslCtx{nullptr, ::SSL_CTX_free};
  bool m_isRuning{false};  // aotmic is unnecessary
};

class TcpTransport;

using BufferEventDataCallback = void (*)(BufferEvent* event, TcpTransport* transport);
using BufferEventEventCallback = void (*)(BufferEvent* event, short what, TcpTransport* transport);

class BufferEvent : public noncopyable {
 public:
  virtual ~BufferEvent();

  void setCallback(BufferEventDataCallback readCallback,
                   BufferEventDataCallback writeCallback,
                   BufferEventEventCallback eventCallback,
                   std::shared_ptr<TcpTransport> transport);

  void setWatermark(short events, size_t lowmark, size_t highmark) {
    bufferevent_setwatermark(m_bufferEvent, events, lowmark, highmark);
  }

  int enable(short event) { return bufferevent_enable(m_bufferEvent, event); }

  int connect(const struct sockaddr* addr, int socklen) {
    return bufferevent_socket_connect(m_bufferEvent, (struct sockaddr*)addr, socklen);
  }

  int write(const void* data, size_t size) { return bufferevent_write(m_bufferEvent, data, size); }

  size_t read(void* data, size_t size) { return bufferevent_read(m_bufferEvent, data, size); }

  struct evbuffer* getInput() {
    return bufferevent_get_input(m_bufferEvent);
  }

  socket_t getfd() const { return bufferevent_getfd(m_bufferEvent); }

  std::string getPeerAddrPort() const { return m_peerAddrPort; }

 private:
  BufferEvent(struct bufferevent* event, bool unlockCallbacks);
  friend EventLoop;

  static void read_callback(struct bufferevent* bev, void* ctx);
  static void write_callback(struct bufferevent* bev, void* ctx);
  static void event_callback(struct bufferevent* bev, short what, void* ctx);

 private:
  struct bufferevent* m_bufferEvent;
  const bool m_unlockCallbacks;

  BufferEventDataCallback m_readCallback;
  BufferEventDataCallback m_writeCallback;
  BufferEventEventCallback m_eventCallback;
  std::weak_ptr<TcpTransport> m_callbackTransport;  // avoid reference cycle

  // cache properties
  std::string m_peerAddrPort;
};

}  // namespace rocketmq

#endif  //__EVENTLOOP_H__
