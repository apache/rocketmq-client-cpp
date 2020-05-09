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

#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/event.h>

#include <functional>
#include <memory>

#include "concurrent/thread.hpp"
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

  bool isRunning() { return _is_running; }

  BufferEvent* createBufferEvent(socket_t fd, int options);

 private:
  void runLoop();

 private:
  struct event_base* m_eventBase;
  thread m_loopThread;

  bool _is_running;  // aotmic is unnecessary
};

class TcpTransport;

class BufferEvent : public noncopyable {
 public:
  typedef std::function<void(BufferEvent& event)> DataCallback;
  typedef std::function<void(BufferEvent& event, short what)> EventCallback;

 private:
  BufferEvent(struct bufferevent* event, bool unlockCallbacks, EventLoop* loop);
  friend EventLoop;

 public:
  virtual ~BufferEvent();

  void setCallback(DataCallback readCallback, DataCallback writeCallback, EventCallback eventCallback);

  void setWatermark(short events, size_t lowmark, size_t highmark) {
    bufferevent_setwatermark(m_bufferEvent, events, lowmark, highmark);
  }

  int enable(short event) { return bufferevent_enable(m_bufferEvent, event); }
  int disable(short event) { return bufferevent_disable(m_bufferEvent, event); }

  int connect(const std::string& addr);
  void close();

  int write(const void* data, size_t size) { return bufferevent_write(m_bufferEvent, data, size); }

  size_t read(void* data, size_t size) { return bufferevent_read(m_bufferEvent, data, size); }

  struct evbuffer* getInput() {
    return bufferevent_get_input(m_bufferEvent);
  }

  socket_t getfd() const { return bufferevent_getfd(m_bufferEvent); }

  const std::string& getPeerAddrPort() const { return m_peerAddrPort; }

 private:
  static void read_callback(struct bufferevent* bev, void* ctx);
  static void write_callback(struct bufferevent* bev, void* ctx);
  static void event_callback(struct bufferevent* bev, short what, void* ctx);

 private:
  EventLoop* m_eventLoop;
  struct bufferevent* m_bufferEvent;
  const bool m_unlockCallbacks;

  DataCallback m_readCallback;
  DataCallback m_writeCallback;
  EventCallback m_eventCallback;

  // cached properties
  std::string m_peerAddrPort;
};

}  // namespace rocketmq

#endif  // __EVENTLOOP_H__
