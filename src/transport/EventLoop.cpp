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
#include "EventLoop.h"

#include <event2/thread.h>

#include "Logging.h"
#include "SocketUtil.h"

namespace rocketmq {

EventLoop* EventLoop::GetDefaultEventLoop() {
  static EventLoop defaultEventLoop;
  return &defaultEventLoop;
}

EventLoop::EventLoop(const struct event_config* config, bool run_immediately)
    : event_base_(nullptr), loop_thread_("EventLoop"), is_running_(false) {
// tell libevent support multi-threads
#ifdef WIN32
  evthread_use_windows_threads();
#else
  evthread_use_pthreads();
#endif

  if (config == nullptr) {
    event_base_ = event_base_new();
  } else {
    event_base_ = event_base_new_with_config(config);
  }

  if (event_base_ == nullptr) {
    // FIXME: failure...
    LOG_ERROR_NEW("[CRITICAL] Failed to create event base!");
    exit(-1);
  }

  evthread_make_base_notifiable(event_base_);

  loop_thread_.set_target(&EventLoop::runLoop, this);

  if (run_immediately) {
    start();
  }
}

EventLoop::~EventLoop() {
  stop();

  if (event_base_ != nullptr) {
    event_base_free(event_base_);
    event_base_ = nullptr;
  }
}

void EventLoop::start() {
  if (!is_running_) {
    is_running_ = true;
    loop_thread_.start();
  }
}

void EventLoop::stop() {
  if (is_running_) {
    is_running_ = false;
    loop_thread_.join();
  }
}

void EventLoop::runLoop() {
  while (is_running_) {
    int ret = event_base_dispatch(event_base_);
    if (ret == 1) {
      // no event
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  }
}

#define OPT_UNLOCK_CALLBACKS (BEV_OPT_DEFER_CALLBACKS | BEV_OPT_UNLOCK_CALLBACKS)

BufferEvent* EventLoop::createBufferEvent(socket_t fd, int options) {
  struct bufferevent* event = bufferevent_socket_new(event_base_, fd, options);
  if (event == nullptr) {
    auto ev_errno = EVUTIL_SOCKET_ERROR();
    LOG_ERROR_NEW("create bufferevent failed: {}", evutil_socket_error_to_string(ev_errno));
    return nullptr;
  }

  bool unlock = (options & OPT_UNLOCK_CALLBACKS) == OPT_UNLOCK_CALLBACKS;

  return new BufferEvent(event, unlock, this);
}

BufferEvent::BufferEvent(struct bufferevent* event, bool unlockCallbacks, EventLoop* loop)
    : event_loop_(loop),
      buffer_event_(event),
      unlock_callbacks_(unlockCallbacks),
      read_callback_(nullptr),
      write_callback_(nullptr),
      event_callback_(nullptr) {
  if (buffer_event_ != nullptr) {
    bufferevent_incref(buffer_event_);
  }
}

BufferEvent::~BufferEvent() {
  if (buffer_event_ != nullptr) {
    bufferevent_decref(buffer_event_);
  }
}

void BufferEvent::setCallback(DataCallback readCallback, DataCallback writeCallback, EventCallback eventCallback) {
  if (buffer_event_ == nullptr) {
    return;
  }

  // use lock in bufferevent
  bufferevent_lock(buffer_event_);

  // wrap callback
  read_callback_ = readCallback;
  write_callback_ = writeCallback;
  event_callback_ = eventCallback;

  bufferevent_data_cb readcb = readCallback != nullptr ? read_callback : nullptr;
  bufferevent_data_cb writecb = writeCallback != nullptr ? write_callback : nullptr;
  bufferevent_event_cb eventcb = eventCallback != nullptr ? event_callback : nullptr;

  bufferevent_setcb(buffer_event_, readcb, writecb, eventcb, this);

  bufferevent_unlock(buffer_event_);
}

void BufferEvent::read_callback(struct bufferevent* bev, void* ctx) {
  auto event = static_cast<BufferEvent*>(ctx);

  if (event->unlock_callbacks_) {
    bufferevent_lock(event->buffer_event_);
  }

  auto callback = event->read_callback_;

  if (event->unlock_callbacks_) {
    bufferevent_unlock(event->buffer_event_);
  }

  if (callback != nullptr) {
    callback(*event);
  }
}

void BufferEvent::write_callback(struct bufferevent* bev, void* ctx) {
  auto event = static_cast<BufferEvent*>(ctx);

  if (event->unlock_callbacks_) {
    bufferevent_lock(event->buffer_event_);
  }

  auto callback = event->write_callback_;

  if (event->unlock_callbacks_) {
    bufferevent_unlock(event->buffer_event_);
  }

  if (callback != nullptr) {
    callback(*event);
  }
}

void BufferEvent::event_callback(struct bufferevent* bev, short what, void* ctx) {
  auto event = static_cast<BufferEvent*>(ctx);

  if (event->unlock_callbacks_) {
    bufferevent_lock(event->buffer_event_);
  }

  auto callback = event->event_callback_;

  if (event->unlock_callbacks_) {
    bufferevent_unlock(event->buffer_event_);
  }

  if (callback != nullptr) {
    callback(*event, what);
  }
}

int BufferEvent::connect(const std::string& addr) {
  if (buffer_event_ == nullptr) {
    LOG_WARN_NEW("have not bufferevent object to connect {}", addr);
    return -1;
  }

  try {
    auto* sa = StringToSockaddr(addr);  // resolve domain
    peer_addr_port_ = SockaddrToString(sa);
    return bufferevent_socket_connect(buffer_event_, sa, SockaddrSize(sa));
  } catch (const std::exception& e) {
    LOG_ERROR_NEW("can not connect to {}, {}", addr, e.what());
    return -1;
  }
}

void BufferEvent::close() {
  if (buffer_event_ != nullptr) {
    bufferevent_free(buffer_event_);
  }
}

}  // namespace rocketmq
