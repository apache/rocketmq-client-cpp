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

TcpTransport::TcpTransport(ReadCallback readCallback,
                           CloseCallback closeCallback,
                           std::unique_ptr<TcpTransportInfo> info)
    : event_(nullptr),
      tcp_connect_status_(TCP_CONNECT_STATUS_CREATED),
      status_mutex_(),
      status_event_(),
      read_callback_(readCallback),
      close_callback_(closeCallback),
      info_(std::move(info)) {
  start_time_ = UtilAll::currentTimeMillis();
}

TcpTransport::~TcpTransport() {
  closeBufferEvent(true);
}

TcpConnectStatus TcpTransport::closeBufferEvent(bool isDeleted) {
  // closeBufferEvent is idempotent.
  if (setTcpConnectEvent(TCP_CONNECT_STATUS_CLOSED) != TCP_CONNECT_STATUS_CLOSED) {
    if (event_ != nullptr) {
      event_->close();
    }
    if (!isDeleted && close_callback_ != nullptr) {
      close_callback_(shared_from_this());
    }
  }
  return TCP_CONNECT_STATUS_CLOSED;
}

TcpConnectStatus TcpTransport::getTcpConnectStatus() {
  return tcp_connect_status_;
}

TcpConnectStatus TcpTransport::waitTcpConnectEvent(int timeoutMillis) {
  if (tcp_connect_status_ == TCP_CONNECT_STATUS_CONNECTING) {
    std::unique_lock<std::mutex> lock(status_mutex_);
    if (!status_event_.wait_for(lock, std::chrono::milliseconds(timeoutMillis),
                                [&] { return tcp_connect_status_ != TCP_CONNECT_STATUS_CONNECTING; })) {
      LOG_INFO("connect timeout");
    }
  }
  return tcp_connect_status_;
}

// internal method
TcpConnectStatus TcpTransport::setTcpConnectEvent(TcpConnectStatus connectStatus) {
  TcpConnectStatus oldStatus = tcp_connect_status_.exchange(connectStatus, std::memory_order_relaxed);
  if (oldStatus == TCP_CONNECT_STATUS_CONNECTING) {
    // awake waiting thread
    status_event_.notify_all();
  }
  return oldStatus;
}

bool TcpTransport::setTcpConnectEventIf(TcpConnectStatus& expectedStatus, TcpConnectStatus newStatus) {
  bool isSuccessed = tcp_connect_status_.compare_exchange_strong(expectedStatus, newStatus);
  if (expectedStatus == TCP_CONNECT_STATUS_CONNECTING) {
    // awake waiting thread
    status_event_.notify_all();
  }
  return isSuccessed;
}

void TcpTransport::disconnect(const std::string& addr) {
  // disconnect is idempotent.
  LOG_INFO_NEW("disconnect:{} start. event:{}", addr, (void*)event_.get());
  closeBufferEvent();
  LOG_INFO_NEW("disconnect:{} completely", addr);
}

TcpConnectStatus TcpTransport::connect(const std::string& addr, int timeoutMillis) {
  LOG_INFO_NEW("connect to {}.", addr);

  TcpConnectStatus curStatus = TCP_CONNECT_STATUS_CREATED;
  if (setTcpConnectEventIf(curStatus, TCP_CONNECT_STATUS_CONNECTING)) {
    // create BufferEvent
    event_.reset(EventLoop::GetDefaultEventLoop()->createBufferEvent(-1, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_THREADSAFE));
    if (nullptr == event_) {
      LOG_ERROR_NEW("create BufferEvent failed");
      return closeBufferEvent();
    }

    // then, configure BufferEvent
    auto weak_this = std::weak_ptr<TcpTransport>(shared_from_this());
    auto readCallback = [=](BufferEvent& event) {
      auto channel = weak_this.lock();
      if (channel != nullptr) {
        channel->dataArrived(event);
      } else {
        LOG_WARN_NEW("[BUG] TcpTransport object is released.");
      }
    };
    auto eventCallback = [=](BufferEvent& event, short what) {
      auto channel = weak_this.lock();
      if (channel != nullptr) {
        channel->eventOccurred(event, what);
      } else {
        LOG_WARN_NEW("[BUG] TcpTransport object is released.");
      }
    };
    event_->setCallback(readCallback, nullptr, eventCallback);
    event_->setWatermark(EV_READ, 4, 0);
    event_->enable(EV_READ | EV_WRITE);

    if (event_->connect(addr) < 0) {
      LOG_WARN_NEW("connect to fd:{} failed", event_->getfd());
      return closeBufferEvent();
    }
  } else {
    return curStatus;
  }

  if (timeoutMillis <= 0) {
    LOG_INFO_NEW("try to connect to fd:{}, addr:{}", event_->getfd(), addr);
    return TCP_CONNECT_STATUS_CONNECTING;
  }

  TcpConnectStatus connectStatus = waitTcpConnectEvent(timeoutMillis);
  if (connectStatus != TCP_CONNECT_STATUS_CONNECTED) {
    LOG_WARN_NEW("can not connect to server:{}", addr);
    return closeBufferEvent();
  }

  return TCP_CONNECT_STATUS_CONNECTED;
}

void TcpTransport::eventOccurred(BufferEvent& event, short what) {
  socket_t fd = event.getfd();
  LOG_INFO_NEW("eventcb: received event:0x{:X} on fd:{}", what, fd);

  if (what & BEV_EVENT_CONNECTED) {
    LOG_INFO_NEW("eventcb: connect to fd:{} successfully", fd);

    // disable Nagle
    int val = 1;
    if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (void*)&val, sizeof(val)) < 0) {
      LOG_WARN_NEW("eventcb: disable Nagle failed. fd:{}", fd);
    }

    // disable Keep-Alive
    val = 0;
    if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (void*)&val, sizeof(val)) < 0) {
      LOG_WARN_NEW("eventcb: disable Keep-Alive failed. fd:{}", fd);
    }

    TcpConnectStatus curStatus = TCP_CONNECT_STATUS_CONNECTING;
    setTcpConnectEventIf(curStatus, TCP_CONNECT_STATUS_CONNECTED);
  } else if (what & (BEV_EVENT_ERROR | BEV_EVENT_EOF)) {
    LOG_ERROR_NEW("eventcb: received error event:0x{:X} on fd:{}", what, fd);
    closeBufferEvent();
    // if error, stop callback.
    // TcpConnectStatus curStatus = getTcpConnectStatus();
    // while (curStatus != TCP_CONNECT_STATUS_CLOSED && curStatus != TCP_CONNECT_STATUS_FAILED) {
    //   if (setTcpConnectEventIf(curStatus, TCP_CONNECT_STATUS_FAILED)) {
    //     event.setCallback(nullptr, nullptr, nullptr);
    //     break;
    //   }
    //   curStatus = getTcpConnectStatus();
    // }
  } else {
    LOG_ERROR_NEW("eventcb: received error event:0x{:X} on fd:{}", what, fd);
  }
}

void TcpTransport::dataArrived(BufferEvent& event) {
  /* This callback is invoked when there is data to read on bev. */

  // protocol:  <length> <header length> <header data> <body data>
  //               1            2               3           4
  // rocketmq protocol contains 4 parts as following:
  //     1, big endian 4 bytes int, its length is sum of 2,3 and 4
  //     2, big endian 4 bytes int, its length is 3
  //     3, use json to serialization data
  //     4, application could self-defined binary data

  struct evbuffer* input = event.getInput();
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

    uint32_t msgLen = ByteOrderUtil::NorminalBigEndian(packageLength);  // same as ntohl()
    size_t recvLen = evbuffer_get_length(input);
    if (recvLen >= msgLen + 4) {
      LOG_DEBUG_NEW("had received all data. msgLen:{}, from:{}, recvLen:{}", msgLen, event.getfd(), recvLen);
    } else {
      LOG_DEBUG_NEW("didn't received whole. msgLen:{}, from:{}, recvLen:{}", msgLen, event.getfd(), recvLen);
      return;  // consider large data which was not received completely by now
    }

    if (msgLen > 0) {
      auto msg = std::make_shared<ByteArray>(msgLen);

      event.read(&packageLength, 4);  // skip length field
      event.read(msg->array(), msgLen);

      messageReceived(std::move(msg));
    }
  }
}

void TcpTransport::messageReceived(ByteArrayRef msg) {
  if (read_callback_ != nullptr) {
    read_callback_(std::move(msg), shared_from_this());
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
  return event_ != nullptr && event_->write(data, len) == 0;
}

const std::string& TcpTransport::getPeerAddrAndPort() {
  return event_ != nullptr ? event_->getPeerAddrPort() : null;
}

const uint64_t TcpTransport::getStartTime() const {
  return start_time_;
}

}  // namespace rocketmq
