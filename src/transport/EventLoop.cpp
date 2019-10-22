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

#ifndef WIN32
#include <arpa/inet.h>
#else
#include <Winsock2.h>
#endif

#include "Logging.h"
#include "SocketUtil.h"
#include "UtilAll.h"

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
    LOG_ERROR("Failed to create event base!");
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
    int ret;

    ret = event_base_dispatch(m_eventBase);
    // ret = event_base_loop(m_eventBase, EVLOOP_NONBLOCK);

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

  return new BufferEvent(event, unlock);
}

BufferEvent::BufferEvent(struct bufferevent* event, bool unlockCallbacks)
    : m_bufferEvent(event),
      m_unlockCallbacks(unlockCallbacks),
      m_readCallback(nullptr),
      m_writeCallback(nullptr),
      m_eventCallback(nullptr),
      m_callbackTransport() {
#ifdef ROCKETMQ_BUFFEREVENT_PROXY_ALL_CALLBACK
  if (m_bufferEvent != nullptr) {
    bufferevent_setcb(m_bufferEvent, read_callback, write_callback, event_callback, this);
  }
#endif  // ROCKETMQ_BUFFEREVENT_PROXY_ALL_CALLBACK
}

BufferEvent::~BufferEvent() {
  if (m_bufferEvent != nullptr) {
    // free function will set all callbacks to NULL first.
    bufferevent_free(m_bufferEvent);
    m_bufferEvent = nullptr;
  }
}

void BufferEvent::setCallback(BufferEventDataCallback readCallback,
                              BufferEventDataCallback writeCallback,
                              BufferEventEventCallback eventCallback,
                              std::shared_ptr<TcpTransport> transport) {
  // use lock in bufferevent
  bufferevent_lock(m_bufferEvent);

  // wrap callback
  m_readCallback = readCallback;
  m_writeCallback = writeCallback;
  m_eventCallback = eventCallback;
  m_callbackTransport = transport;

#ifndef ROCKETMQ_BUFFEREVENT_PROXY_ALL_CALLBACK
  bufferevent_data_cb readcb = readCallback != nullptr ? read_callback : nullptr;
  bufferevent_data_cb writecb = writeCallback != nullptr ? write_callback : nullptr;
  bufferevent_event_cb eventcb = eventCallback != nullptr ? event_callback : nullptr;

  bufferevent_setcb(m_bufferEvent, readcb, writecb, eventcb, this);
#endif  // ROCKETMQ_BUFFEREVENT_PROXY_ALL_CALLBACK

  bufferevent_unlock(m_bufferEvent);
}

void BufferEvent::read_callback(struct bufferevent* bev, void* ctx) {
  auto event = static_cast<BufferEvent*>(ctx);

  if (event->m_unlockCallbacks) {
    bufferevent_lock(event->m_bufferEvent);
  }

  BufferEventDataCallback callback = event->m_readCallback;
  std::shared_ptr<TcpTransport> transport = event->m_callbackTransport.lock();

  if (event->m_unlockCallbacks) {
    bufferevent_unlock(event->m_bufferEvent);
  }

  if (callback != nullptr) {
    callback(event, transport.get());
  }
}

void BufferEvent::write_callback(struct bufferevent* bev, void* ctx) {
  auto event = static_cast<BufferEvent*>(ctx);

  if (event->m_unlockCallbacks) {
    bufferevent_lock(event->m_bufferEvent);
  }

  BufferEventDataCallback callback = event->m_writeCallback;
  std::shared_ptr<TcpTransport> transport = event->m_callbackTransport.lock();

  if (event->m_unlockCallbacks) {
    bufferevent_unlock(event->m_bufferEvent);
  }

  if (callback != nullptr) {
    callback(event, transport.get());
  }
}

static std::string buildPeerAddrPort(socket_t fd) {
  sockaddr_in addr;
  socklen_t len = sizeof(addr);

  getpeername(fd, (struct sockaddr*)&addr, &len);

  std::string addrPort = socketAddress2IPPort((struct sockaddr*)&addr);
  LOG_DEBUG("socket: %d, addr: %s", fd, addrPort);

  return addrPort;
}

void BufferEvent::event_callback(struct bufferevent* bev, short what, void* ctx) {
  auto event = static_cast<BufferEvent*>(ctx);

  if (what & BEV_EVENT_CONNECTED) {
    socket_t fd = event->getfd();
    event->m_peerAddrPort = buildPeerAddrPort(fd);
  }

  if (event->m_unlockCallbacks) {
    bufferevent_lock(event->m_bufferEvent);
  }

  BufferEventEventCallback callback = event->m_eventCallback;
  std::shared_ptr<TcpTransport> transport = event->m_callbackTransport.lock();

  if (event->m_unlockCallbacks) {
    bufferevent_unlock(event->m_bufferEvent);
  }

  if (callback != nullptr) {
    callback(event, what, transport.get());
  }
}

}  // namespace rocketmq
