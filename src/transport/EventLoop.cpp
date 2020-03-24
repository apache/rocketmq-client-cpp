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

namespace rocketmq {

EventLoop* EventLoop::GetDefaultEventLoop() {
  static EventLoop defaultEventLoop;
  return &defaultEventLoop;
}

EventLoop::EventLoop(const struct event_config* config, bool run_immediately) {
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
    m_isRuning = false;
    m_loopThread->join();

    delete m_loopThread;
    m_loopThread = nullptr;
  }
}

void EventLoop::runLoop() {
  m_isRuning = true;

  while (m_isRuning) {
    int ret;

    ret = event_base_dispatch(m_eventBase);
    //    ret = event_base_loop(m_eventBase, EVLOOP_NONBLOCK);

    if (ret == 1) {
      // no event
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  }
}

bool EventLoop::CreateSslContext(const std::string& ssl_property_file) {
  ERR_load_crypto_strings();
  SSL_load_error_strings();
  SSL_library_init();
  OpenSSL_add_all_algorithms();

  m_sslCtx.reset(SSL_CTX_new(SSLv23_client_method()));
  if (!m_sslCtx) {
    LOG_ERROR("Failed to create ssl context!");
    return false;
  }

  std::string client_key_file = DEFAULT_CLIENT_KEY_FILE;
  std::string client_key_passwd = DEFAULT_CLIENT_KEY_PASSWD;
  std::string client_cert_file = DEFAULT_CLIENT_CERT_FILE;
  std::string ca_cert_file = DEFAULT_CA_CERT_FILE;
  auto properties = UtilAll::ReadProperties(ssl_property_file);
  if (!properties.empty()) {
    if (properties.find("tls.client.keyPath") != properties.end()) {
      client_key_file = properties["tls.client.keyPath"];
    }
    if (properties.find("tls.client.keyPassword") != properties.end()) {
      client_key_passwd = properties["tls.client.keyPassword"];
    }
    if (properties.find("tls.client.certPath") != properties.end()) {
      client_cert_file = properties["tls.client.certPath"];
    }
    if (properties.find("tls.client.trustCertPath") != properties.end()) {
      ca_cert_file = properties["tls.client.trustCertPath"];
    }
  } else {
    LOG_WARN(
        "The tls properties file is not specified or empty. "
        "Set it by modifying the api of setTlsPropertyFile and fill the configuration content.");
  }

  SSL_CTX_set_verify(m_sslCtx.get(), SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);
  SSL_CTX_set_mode(m_sslCtx.get(), SSL_MODE_AUTO_RETRY);

  if (client_key_passwd.empty()) {
    LOG_WARN(
        "The pass phrase is not specified. "
        "Set it by adding the 'tls.client.keyPassword' property in configuration file.");
  } else {
    SSL_CTX_set_default_passwd_cb_userdata(m_sslCtx.get(), (void*)client_key_passwd.c_str());
  }

  bool check_flag{true};
  if (!boost::filesystem::exists(ca_cert_file.c_str())) {
    check_flag = false;
    LOG_WARN(
        "'%s' does not exist. Please make sure the 'tls.client.trustCertPath' property "
        "in the configuration file is configured correctly.",
        ca_cert_file.c_str());
  } else if (SSL_CTX_load_verify_locations(m_sslCtx.get(), ca_cert_file.c_str(), NULL) <= 0) {
    LOG_ERROR("SSL_CTX_load_verify_locations error!");
    ERR_print_errors_fp(stderr);
    return false;
  }

  if (!boost::filesystem::exists(client_cert_file.c_str())) {
    check_flag = false;
    LOG_WARN(
        "'%s' does not exist. Please make sure the 'tls.client.certPath' property "
        "in the configuration file is configured correctly.",
        client_cert_file.c_str());
  } else if (SSL_CTX_use_certificate_file(m_sslCtx.get(), client_cert_file.c_str(), SSL_FILETYPE_PEM) <= 0) {
    LOG_ERROR("SSL_CTX_use_certificate_file error!");
    ERR_print_errors_fp(stderr);
    return false;
  }

  if (!boost::filesystem::exists(client_key_file.c_str())) {
    check_flag = false;
    LOG_WARN(
        "'%s' does not exist. Please make sure the 'tls.client.keyPath' property "
        "in the configuration file is configured correctly.",
        client_key_file.c_str());
  } else if (SSL_CTX_use_PrivateKey_file(m_sslCtx.get(), client_key_file.c_str(), SSL_FILETYPE_PEM) <= 0) {
    LOG_ERROR("SSL_CTX_use_PrivateKey_file error!");
    ERR_print_errors_fp(stderr);
    return false;
  }

  if (check_flag && SSL_CTX_check_private_key(m_sslCtx.get()) <= 0) {
    LOG_ERROR("SSL_CTX_check_private_key error!");
    ERR_print_errors_fp(stderr);
    return false;
  }

  return true;
}

#define OPT_UNLOCK_CALLBACKS (BEV_OPT_DEFER_CALLBACKS | BEV_OPT_UNLOCK_CALLBACKS)

BufferEvent* EventLoop::createBufferEvent(socket_t fd,
                                          int options,
                                          bool enable_ssl,
                                          const std::string& ssl_property_file) {
  struct bufferevent* event{nullptr};

  if (enable_ssl) {
    if (!m_sslCtx && !CreateSslContext(ssl_property_file)) {
      LOG_ERROR("Failed to create ssl context!");
      return nullptr;
    }

    SSL* ssl = SSL_new(m_sslCtx.get());
    if (ssl == nullptr) {
      LOG_ERROR("Failed to create ssl handle!");
      return nullptr;
    }

    // create ssl bufferevent
    event = bufferevent_openssl_socket_new(m_eventBase, fd, ssl, BUFFEREVENT_SSL_CONNECTING, options);

    /* create filter ssl bufferevent
    struct bufferevent *bev = bufferevent_socket_new(m_eventBase, fd, options);
    event = bufferevent_openssl_filter_new(m_eventBase, bev, ssl,
                                           BUFFEREVENT_SSL_CONNECTING, options);
    */
  } else {
    event = bufferevent_socket_new(m_eventBase, fd, options);
  }

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
