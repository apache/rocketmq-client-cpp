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
#ifndef ROCKETMQ_TRANSPORT_EVENTLOOP_H_
#define ROCKETMQ_TRANSPORT_EVENTLOOP_H_

#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/event.h>

#include <functional>  // std::function

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

  bool isRunning() { return is_running_; }

  BufferEvent* createBufferEvent(socket_t fd, int options);

 private:
  void runLoop();

 private:
  struct event_base* event_base_;
  thread loop_thread_;

  bool is_running_;  // aotmic is unnecessary
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

  inline void setWatermark(short events, size_t lowmark, size_t highmark) {
    bufferevent_setwatermark(buffer_event_, events, lowmark, highmark);
  }

  inline int enable(short event) { return bufferevent_enable(buffer_event_, event); }
  inline int disable(short event) { return bufferevent_disable(buffer_event_, event); }

  int connect(const std::string& addr);
  void close();

  inline int write(const void* data, size_t size) { return bufferevent_write(buffer_event_, data, size); }

  inline size_t read(void* data, size_t size) { return bufferevent_read(buffer_event_, data, size); }

  inline struct evbuffer* getInput() { return bufferevent_get_input(buffer_event_); }

  inline socket_t getfd() const { return bufferevent_getfd(buffer_event_); }

  inline const std::string& getPeerAddrPort() const { return peer_addr_port_; }

 private:
  static void read_callback(struct bufferevent* bev, void* ctx);
  static void write_callback(struct bufferevent* bev, void* ctx);
  static void event_callback(struct bufferevent* bev, short what, void* ctx);

 private:
  EventLoop* event_loop_;
  struct bufferevent* buffer_event_;
  const bool unlock_callbacks_;

  DataCallback read_callback_;
  DataCallback write_callback_;
  EventCallback event_callback_;

  // cached properties
  std::string peer_addr_port_;
};

}  // namespace rocketmq

#endif  // ROCKETMQ_TRANSPORT_EVENTLOOP_H_
