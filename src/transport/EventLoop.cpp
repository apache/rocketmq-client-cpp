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

#if !defined(WIN32) && !defined(__APPLE__)
#include <sys/prctl.h>
#endif

#include <event2/thread.h>

#include <boost/filesystem.hpp>

#include "Logging.h"
#include "UtilAll.h"

namespace rocketmq {

EventLoop* EventLoop::GetDefaultEventLoop() {
  static EventLoop defaultEventLoop;
  return &defaultEventLoop;
}

EventLoop::EventLoop(const struct event_config* config, bool run_immediately) {
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
    // failure...
    LOG_ERROR("Failed to create event base!");
    return;
  }

  evthread_make_base_notifiable(m_eventBase);

#ifdef ENABLE_OPENSSL
  if (!CreateSslContext()) {
    LOG_ERROR("Failed to create ssl context!");
    return;
  }
#endif

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
  if (m_loopThread == nullptr) {
    // start event loop
#if !defined(WIN32) && !defined(__APPLE__)
    string taskName = UtilAll::getProcessName();
    prctl(PR_SET_NAME, "EventLoop", 0, 0, 0);
#endif
    m_loopThread = new std::thread(&EventLoop::runLoop, this);
#if !defined(WIN32) && !defined(__APPLE__)
    prctl(PR_SET_NAME, taskName.c_str(), 0, 0, 0);
#endif
  }
}

void EventLoop::stop() {
  if (m_loopThread != nullptr /*&& m_loopThread.joinable()*/) {
    _is_running = false;
    m_loopThread->join();

    delete m_loopThread;
    m_loopThread = nullptr;
  }
}

void EventLoop::runLoop() {
  _is_running = true;

  while (_is_running) {
    int ret;

    ret = event_base_dispatch(m_eventBase);
    //    ret = event_base_loop(m_eventBase, EVLOOP_NONBLOCK);

    if (ret == 1) {
      // no event
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  }
}

#ifdef ENABLE_OPENSSL
bool EventLoop::CreateSslContext() {
  ERR_load_crypto_strings();
  SSL_load_error_strings();
  SSL_library_init();
  OpenSSL_add_all_algorithms();

  m_ssl_ctx.reset(SSL_CTX_new(SSLv23_client_method()));
  if (m_ssl_ctx.get() == nullptr) {
    LOG_ERROR("Failed to create ssl context!");
    return false;
  }

  const char* CA_CERT_FILE_DEFAULT     = "/etc/rocketmq/ca.pem";
  const char* CLIENT_CERT_FILE_DEFAULT = "/etc/rocketmq/client.pem";
  const char* CLIENT_KEY_FILE_DEFAULT  = "/etc/rocketmq/client.key";

  const char* CA_CERT_FILE     = std::getenv("CA_CERT_FILE");
  const char* CLIENT_CERT_FILE = std::getenv("CLIENT_CERT_FILE");
  const char* CLIENT_KEY_FILE  = std::getenv("CLIENT_KEY_FILE");
  const char* PASS_PHRASE      = std::getenv("PASS_PHRASE");

  if (!CA_CERT_FILE)     { CA_CERT_FILE     = CA_CERT_FILE_DEFAULT; }
  if (!CLIENT_CERT_FILE) { CLIENT_CERT_FILE = CLIENT_CERT_FILE_DEFAULT; }
  if (!CLIENT_KEY_FILE)  { CLIENT_KEY_FILE  = CLIENT_KEY_FILE_DEFAULT; }

  SSL_CTX_set_verify(m_ssl_ctx.get(), SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);  
  SSL_CTX_set_mode(m_ssl_ctx.get(), SSL_MODE_AUTO_RETRY);

  if (!PASS_PHRASE) {
    LOG_WARN("The pass phrase is not specified. Set it by modifying the environment variable 'PASS_PHRASE'.", CA_CERT_FILE);
  } else {
    SSL_CTX_set_default_passwd_cb_userdata(m_ssl_ctx.get(), (void*)PASS_PHRASE);
  }

  bool check_flag { true };
  if (!boost::filesystem::exists(CA_CERT_FILE)) {
    check_flag = false;
    LOG_WARN("'%s' does not exist. Please apply for a certificate from the relevant authority "
        "or modify the environment variable 'CA_CERT_FILE' to point to its location.", CA_CERT_FILE);
  } else if (SSL_CTX_load_verify_locations(m_ssl_ctx.get(), CA_CERT_FILE, NULL) <= 0) {
    LOG_ERROR("SSL_CTX_load_verify_locations error!");
    ERR_print_errors_fp(stderr);
    return false;
  }

  if (!boost::filesystem::exists(CLIENT_CERT_FILE)) {
    check_flag = false;
    LOG_WARN("'%s' does not exist. Please apply for a certificate from the relevant authority "
        "or modify the environment variable 'CLIENT_CERT_FILE' to point to its location.", CLIENT_CERT_FILE);
  } else if (SSL_CTX_use_certificate_file(m_ssl_ctx.get(), CLIENT_CERT_FILE, SSL_FILETYPE_PEM) <= 0) {
    LOG_ERROR("SSL_CTX_use_certificate_file error!");
    ERR_print_errors_fp(stderr);
    return false;
  }

  if (!boost::filesystem::exists(CLIENT_KEY_FILE)) {
    check_flag = false;
    LOG_WARN("'%s' does not exist. Please apply for a certificate from the relevant authority "
        "or modify the environment variable 'CLIENT_KEY_FILE' to point to its location.", CLIENT_KEY_FILE);
  } else if (SSL_CTX_use_PrivateKey_file(m_ssl_ctx.get(), CLIENT_KEY_FILE, SSL_FILETYPE_PEM) <= 0) {
    LOG_ERROR("SSL_CTX_use_PrivateKey_file error!");
    ERR_print_errors_fp(stderr);
    return false;
  }

  if(check_flag && SSL_CTX_check_private_key(m_ssl_ctx.get()) <= 0)
  {
    LOG_ERROR("SSL_CTX_check_private_key error!");
    ERR_print_errors_fp(stderr);
    return false;
  }

  return true;
}
#endif

#define OPT_UNLOCK_CALLBACKS (BEV_OPT_DEFER_CALLBACKS | BEV_OPT_UNLOCK_CALLBACKS)

BufferEvent* EventLoop::createBufferEvent(socket_t fd, int options) {

#ifdef ENABLE_OPENSSL
  SSL* ssl = SSL_new(m_ssl_ctx.get());
  if (ssl == nullptr) {
    LOG_ERROR("Failed to create ssl handle!");
    return nullptr;
  }

  // create ssl bufferevent
  struct bufferevent* event = bufferevent_openssl_socket_new(m_eventBase, fd, ssl,
                                                             BUFFEREVENT_SSL_CONNECTING, options);
  
  /* create filter ssl bufferevent 
  struct bufferevent *bev = bufferevent_socket_new(m_eventBase, fd, options);
  struct bufferevent* event = bufferevent_openssl_filter_new(m_eventBase, bev, ssl,
                                                             BUFFEREVENT_SSL_CONNECTING, options);
  */
#else
  struct bufferevent* event = bufferevent_socket_new(m_eventBase, fd, options);
#endif

  if (event == nullptr) {
    LOG_ERROR("Failed to create bufferevent!");
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

  if (event->m_unlockCallbacks)
    bufferevent_lock(event->m_bufferEvent);

  BufferEventDataCallback callback = event->m_readCallback;
  std::shared_ptr<TcpTransport> transport = event->m_callbackTransport.lock();

  if (event->m_unlockCallbacks)
    bufferevent_unlock(event->m_bufferEvent);

  if (callback) {
    callback(event, transport.get());
  }
}

void BufferEvent::write_callback(struct bufferevent* bev, void* ctx) {
  auto event = static_cast<BufferEvent*>(ctx);

  if (event->m_unlockCallbacks)
    bufferevent_lock(event->m_bufferEvent);

  BufferEventDataCallback callback = event->m_writeCallback;
  std::shared_ptr<TcpTransport> transport = event->m_callbackTransport.lock();

  if (event->m_unlockCallbacks)
    bufferevent_unlock(event->m_bufferEvent);

  if (callback) {
    callback(event, transport.get());
  }
}

static std::string buildPeerAddrPort(socket_t fd) {
  sockaddr_in addr;
  socklen_t len = sizeof(addr);

  getpeername(fd, (struct sockaddr*)&addr, &len);

  LOG_DEBUG("socket: %d, addr: %s, port: %d", fd, inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
  std::string addrPort(inet_ntoa(addr.sin_addr));
  addrPort.append(":");
  addrPort.append(UtilAll::to_string(ntohs(addr.sin_port)));

  return addrPort;
}

void BufferEvent::event_callback(struct bufferevent* bev, short what, void* ctx) {
  auto event = static_cast<BufferEvent*>(ctx);

  if (what & BEV_EVENT_CONNECTED) {
    socket_t fd = event->getfd();
    event->m_peerAddrPort = buildPeerAddrPort(fd);
  }

  if (event->m_unlockCallbacks)
    bufferevent_lock(event->m_bufferEvent);

  BufferEventEventCallback callback = event->m_eventCallback;
  std::shared_ptr<TcpTransport> transport = event->m_callbackTransport.lock();

  if (event->m_unlockCallbacks)
    bufferevent_unlock(event->m_bufferEvent);

  if (callback) {
    callback(event, what, transport.get());
  }
}

}  // namespace rocketmq
