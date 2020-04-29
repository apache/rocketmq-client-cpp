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
    : m_eventBase(nullptr), m_loopThread("EventLoop"), _is_running(false) {
  // tell libevent support multi-threads
#ifdef WIN32
  evthread_use_windows_threads();
#else
  evthread_use_pthreads();
#endif

  if (config == nullptr) {
    m_eventBase = event_base_new();
  } else {
    m_eventBase = event_base_new_with_config(config);
  }

  if (m_eventBase == nullptr) {
    // FIXME: failure...
    LOG_ERROR_NEW("[CRITICAL] Failed to create event base!");
    exit(-1);
  }

  evthread_make_base_notifiable(m_eventBase);

  m_loopThread.set_target(&EventLoop::runLoop, this);

  if (run_immediately) {
    start();
  }
}

EventLoop::~EventLoop() {
  stop();

  if (m_eventBase != nullptr) {
    event_base_free(m_eventBase);
    m_eventBase = nullptr;
  }
}

void EventLoop::start() {
  if (!_is_running) {
    _is_running = true;
    m_loopThread.start();
  }
}

void EventLoop::stop() {
  if (_is_running) {
    _is_running = false;
    m_loopThread.join();
  }
}

void EventLoop::runLoop() {
  while (_is_running) {
    int ret = event_base_dispatch(m_eventBase);
    if (ret == 1) {
      // no event
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  }
}

#define OPT_UNLOCK_CALLBACKS (BEV_OPT_DEFER_CALLBACKS | BEV_OPT_UNLOCK_CALLBACKS)

BufferEvent* EventLoop::createBufferEvent(socket_t fd, int options) {
  struct bufferevent* event = bufferevent_socket_new(m_eventBase, fd, options);
  if (event == nullptr) {
    auto ev_errno = EVUTIL_SOCKET_ERROR();
    LOG_ERROR_NEW("create bufferevent failed: {}", evutil_socket_error_to_string(ev_errno));
    return nullptr;
  }

  bool unlock = (options & OPT_UNLOCK_CALLBACKS) == OPT_UNLOCK_CALLBACKS;

  return new BufferEvent(event, unlock, this);
}

BufferEvent::BufferEvent(struct bufferevent* event, bool unlockCallbacks, EventLoop* loop)
    : m_eventLoop(loop),
      m_bufferEvent(event),
      m_unlockCallbacks(unlockCallbacks),
      m_readCallback(nullptr),
      m_writeCallback(nullptr),
      m_eventCallback(nullptr) {
  if (m_bufferEvent != nullptr) {
    bufferevent_incref(m_bufferEvent);
  }
}

BufferEvent::~BufferEvent() {
  if (m_bufferEvent != nullptr) {
    bufferevent_decref(m_bufferEvent);
  }
}

void BufferEvent::setCallback(DataCallback readCallback, DataCallback writeCallback, EventCallback eventCallback) {
  if (m_bufferEvent == nullptr) {
    return;
  }

  // use lock in bufferevent
  bufferevent_lock(m_bufferEvent);

  // wrap callback
  m_readCallback = readCallback;
  m_writeCallback = writeCallback;
  m_eventCallback = eventCallback;

  bufferevent_data_cb readcb = readCallback != nullptr ? read_callback : nullptr;
  bufferevent_data_cb writecb = writeCallback != nullptr ? write_callback : nullptr;
  bufferevent_event_cb eventcb = eventCallback != nullptr ? event_callback : nullptr;

  bufferevent_setcb(m_bufferEvent, readcb, writecb, eventcb, this);

  bufferevent_unlock(m_bufferEvent);
}

void BufferEvent::read_callback(struct bufferevent* bev, void* ctx) {
  auto event = static_cast<BufferEvent*>(ctx);

  if (event->m_unlockCallbacks) {
    bufferevent_lock(event->m_bufferEvent);
  }

  auto callback = event->m_readCallback;

  if (event->m_unlockCallbacks) {
    bufferevent_unlock(event->m_bufferEvent);
  }

  if (callback != nullptr) {
    callback(*event);
  }
}

void BufferEvent::write_callback(struct bufferevent* bev, void* ctx) {
  auto event = static_cast<BufferEvent*>(ctx);

  if (event->m_unlockCallbacks) {
    bufferevent_lock(event->m_bufferEvent);
  }

  auto callback = event->m_writeCallback;

  if (event->m_unlockCallbacks) {
    bufferevent_unlock(event->m_bufferEvent);
  }

  if (callback != nullptr) {
    callback(*event);
  }
}

void BufferEvent::event_callback(struct bufferevent* bev, short what, void* ctx) {
  auto event = static_cast<BufferEvent*>(ctx);

  if (event->m_unlockCallbacks) {
    bufferevent_lock(event->m_bufferEvent);
  }

  auto callback = event->m_eventCallback;

  if (event->m_unlockCallbacks) {
    bufferevent_unlock(event->m_bufferEvent);
  }

  if (callback != nullptr) {
    callback(*event, what);
  }
}

int BufferEvent::connect(const std::string& addr) {
  if (m_bufferEvent == nullptr) {
    LOG_WARN_NEW("have not bufferevent object to connect {}", addr);
    return -1;
  }

  auto* sa = string2SocketAddress(addr);
  m_peerAddrPort = socketAddress2String(sa);  // resolve domain
  return bufferevent_socket_connect(m_bufferEvent, sa, sockaddr_size(sa));
}

void BufferEvent::close() {
  if (m_bufferEvent != nullptr) {
    bufferevent_free(m_bufferEvent);
  }
}

}  // namespace rocketmq
