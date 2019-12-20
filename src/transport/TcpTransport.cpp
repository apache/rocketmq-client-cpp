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
#include "TcpTransport.h"

#include <chrono>

#ifndef WIN32
#include <arpa/inet.h>  // for sockaddr_in and inet_ntoa...
#include <netinet/tcp.h>
#include <sys/socket.h>  // for socket(), bind(), and connect()...
#endif

#include "ByteOrder.h"
#include "Logging.h"
#include "TcpRemotingClient.h"
#include "UtilAll.h"

namespace rocketmq {

TcpTransport::TcpTransport(TcpRemotingClient* client, TcpTransportReadCallback callback)
    : m_event(nullptr),
      m_tcpConnectStatus(TCP_CONNECT_STATUS_CREATED),
      m_statusMutex(),
      m_statusEvent(),
      m_readCallback(callback),
      m_tcpRemotingClient(client) {
  m_startTime = UtilAll::currentTimeMillis();
}

TcpTransport::~TcpTransport() {
  closeBufferEvent();
  m_readCallback = nullptr;
}

TcpConnectStatus TcpTransport::closeBufferEvent() {
  // closeBufferEvent is idempotent.
  if (setTcpConnectEvent(TCP_CONNECT_STATUS_CLOSED) != TCP_CONNECT_STATUS_CLOSED) {
    if (m_event != nullptr) {
      m_event->disable(EV_READ | EV_WRITE);
      // FIXME: not close the socket!!!
    }
  }
  return TCP_CONNECT_STATUS_CLOSED;
}

TcpConnectStatus TcpTransport::getTcpConnectStatus() {
  return m_tcpConnectStatus;
}

TcpConnectStatus TcpTransport::waitTcpConnectEvent(int timeoutMillis) {
  if (m_tcpConnectStatus == TCP_CONNECT_STATUS_CONNECTING) {
    std::unique_lock<std::mutex> lock(m_statusMutex);
    if (!m_statusEvent.wait_for(lock, std::chrono::milliseconds(timeoutMillis),
                                [&] { return m_tcpConnectStatus != TCP_CONNECT_STATUS_CONNECTING; })) {
      LOG_INFO("connect timeout");
    }
  }
  return m_tcpConnectStatus;
}

// internal method
TcpConnectStatus TcpTransport::setTcpConnectEvent(TcpConnectStatus connectStatus) {
  TcpConnectStatus oldStatus = m_tcpConnectStatus.exchange(connectStatus, std::memory_order_relaxed);
  if (oldStatus == TCP_CONNECT_STATUS_CONNECTING) {
    // awake waiting thread
    m_statusEvent.notify_all();
  }
  return oldStatus;
}

bool TcpTransport::setTcpConnectEventIf(TcpConnectStatus& expectedStatus, TcpConnectStatus newStatus) {
  bool isSuccessed = m_tcpConnectStatus.compare_exchange_strong(expectedStatus, newStatus);
  if (expectedStatus == TCP_CONNECT_STATUS_CONNECTING) {
    // awake waiting thread
    m_statusEvent.notify_all();
  }
  return isSuccessed;
}

u_long TcpTransport::resolveInetAddr(std::string& hostname) {
  // TODO: support ipv6
  u_long addr = inet_addr(hostname.c_str());
  if (INADDR_NONE == addr) {
    auto ip = lookupNameServers(hostname);
    addr = inet_addr(ip.c_str());
  }
  return addr;
}

void TcpTransport::disconnect(const std::string& addr) {
  // disconnect is idempotent.
  LOG_INFO_NEW("disconnect:{} start. event:{}", addr, (void*)m_event.get());
  closeBufferEvent();
  LOG_INFO_NEW("disconnect:{} completely", addr);
}

TcpConnectStatus TcpTransport::connect(const std::string& strServerURL, int timeoutMillis) {
  std::string hostname;
  short port;

  LOG_INFO("connect to [{}].", strServerURL);
  if (!UtilAll::SplitURL(strServerURL, hostname, port)) {
    LOG_ERROR("connect to [{}] failed, Invalid url.", strServerURL);
    return closeBufferEvent();
  }

  TcpConnectStatus curStatus = TCP_CONNECT_STATUS_CREATED;
  if (setTcpConnectEventIf(curStatus, TCP_CONNECT_STATUS_CONNECTING)) {
    // TODO: support ipv6
    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = resolveInetAddr(hostname);
    sin.sin_port = htons(port);

    // create BufferEvent
    m_event.reset(EventLoop::GetDefaultEventLoop()->createBufferEvent(-1, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_THREADSAFE));
    if (nullptr == m_event) {
      LOG_ERROR_NEW("create BufferEvent failed");
      return closeBufferEvent();
    }

    // then, configure BufferEvent
    m_event->setCallback(ReadCallback, nullptr, EventCallback, shared_from_this());
    m_event->setWatermark(EV_READ, 4, 0);
    m_event->enable(EV_READ | EV_WRITE);

    if (m_event->connect((struct sockaddr*)&sin, sizeof(sin)) < 0) {
      LOG_WARN_NEW("connect to fd:{} failed", m_event->getfd());
      return closeBufferEvent();
    }
  } else {
    return curStatus;
  }

  if (timeoutMillis <= 0) {
    LOG_INFO_NEW("try to connect to fd:{}, addr:{}", m_event->getfd(), hostname);
    return TCP_CONNECT_STATUS_CONNECTING;
  }

  TcpConnectStatus connectStatus = waitTcpConnectEvent(timeoutMillis);
  if (connectStatus != TCP_CONNECT_STATUS_CONNECTED) {
    LOG_WARN_NEW("can not connect to server:{}", strServerURL);
    return closeBufferEvent();
  }

  return TCP_CONNECT_STATUS_CONNECTED;
}

void TcpTransport::EventCallback(BufferEvent* event, short what, TcpTransport* transport) {
  socket_t fd = event->getfd();
  LOG_INFO("eventcb: received event:%x on fd:%d", what, fd);
  if (what & BEV_EVENT_CONNECTED) {
    LOG_INFO("eventcb: connect to fd:%d successfully", fd);

    // disable Nagle
    int val = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (void*)&val, sizeof(val));

    TcpConnectStatus curStatus = TCP_CONNECT_STATUS_CONNECTING;
    transport->setTcpConnectEventIf(curStatus, TCP_CONNECT_STATUS_CONNECTED);
  } else if (what & (BEV_EVENT_ERROR | BEV_EVENT_EOF)) {
    LOG_INFO("eventcb: received error event cb:%x on fd:%d", what, fd);
    // if error, stop callback.
    TcpConnectStatus curStatus = transport->getTcpConnectStatus();
    while (curStatus != TCP_CONNECT_STATUS_CLOSED && curStatus != TCP_CONNECT_STATUS_FAILED) {
      if (transport->setTcpConnectEventIf(curStatus, TCP_CONNECT_STATUS_FAILED)) {
        event->setCallback(nullptr, nullptr, nullptr, nullptr);
        break;
      }
      curStatus = transport->getTcpConnectStatus();
    }
  } else {
    LOG_ERROR("eventcb: received error event:%d on fd:%d", what, fd);
  }
}

void TcpTransport::ReadCallback(BufferEvent* event, TcpTransport* transport) {
  /* This callback is invoked when there is data to read on bev. */

  // protocol:  <length> <header length> <header data> <body data>
  //               1            2               3           4
  // rocketmq protocol contains 4 parts as following:
  //     1, big endian 4 bytes int, its length is sum of 2,3 and 4
  //     2, big endian 4 bytes int, its length is 3
  //     3, use json to serialization data
  //     4, application could self-defined binary data

  struct evbuffer* input = event->getInput();
  while (1) {
    // glance at first 4 byte to get package length
    struct evbuffer_iovec v[4];
    int n = evbuffer_peek(input, 4, NULL, v, sizeof(v) / sizeof(v[0]));

    uint32_t packageLength;  // first 4 bytes, which indicates 1st part of protocol
    char* p = (char*)&packageLength;
    size_t needed = 4;

    for (int idx = 0; idx < n && needed > 0; idx++) {
      size_t s = needed < v[idx].iov_len ? needed : v[idx].iov_len;
      memcpy(p, v[idx].iov_base, s);
      p += s;
      needed -= s;
    }

    if (needed > 0) {
      LOG_DEBUG_NEW("too little data received with {} byte(s)", 4 - needed);
      return;
    }

    uint32_t msgLen = ByteOrder::swapIfLittleEndian(packageLength);  // same as ntohl()
    size_t recvLen = evbuffer_get_length(input);
    if (recvLen >= msgLen + 4) {
      LOG_DEBUG_NEW("had received all data. msgLen:{}, from:{}, recvLen:{}", msgLen, event->getfd(), recvLen);
    } else {
      LOG_DEBUG_NEW("didn't received whole. msgLen:{}, from:{}, recvLen:{}", msgLen, event->getfd(), recvLen);
      return;  // consider large data which was not received completely by now
    }

    if (msgLen > 0) {
      MemoryBlockPtr3 msg(new MemoryPool(msgLen, true));

      event->read(&packageLength, 4);  // skip length field
      event->read(msg->getData(), msgLen);

      transport->messageReceived(msg, event->getPeerAddrPort());
    }
  }
}

void TcpTransport::messageReceived(MemoryBlockPtr3& mem, const std::string& addr) {
  if (m_readCallback != nullptr) {
    m_readCallback(m_tcpRemotingClient, mem, addr);
  }
}

bool TcpTransport::sendMessage(const char* data, size_t len) {
  if (getTcpConnectStatus() != TCP_CONNECT_STATUS_CONNECTED) {
    return false;
  }

  /* NOTE:
      do not need to consider large data which could not send by once, as
      bufferevent could handle this case;
   */
  return m_event != nullptr && m_event->write(data, len) == 0;
}

const std::string& TcpTransport::getPeerAddrAndPort() {
  return m_event != nullptr ? m_event->getPeerAddrPort() : null;
}

const uint64_t TcpTransport::getStartTime() const {
  return m_startTime;
}

}  // namespace rocketmq
