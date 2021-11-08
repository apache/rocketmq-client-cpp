#include "RemotingSession.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <system_error>

#include "absl/memory/memory.h"
#include "absl/strings/numbers.h"
#include "absl/strings/str_split.h"
#include "spdlog/spdlog.h"

ROCKETMQ_NAMESPACE_BEGIN

void RemotingSession::connect(std::chrono::milliseconds timeout) {
  SessionState expected = SessionState::Created;

  if (state_.compare_exchange_strong(expected, SessionState::Connecting, std::memory_order_relaxed)) {
    auto io_context = context_.lock();
    if (!io_context) {
      state_.store(SessionState::Closing, std::memory_order_relaxed);
      return;
    }

    socket_ = absl::make_unique<asio::ip::tcp::socket>(*io_context);
    std::vector<std::string> segments = absl::StrSplit(endpoint_, ':');

    if (segments.size() != 2) {
      state_.store(SessionState::Closing, std::memory_order_relaxed);
      return;
    }

    asio::error_code ec;
    auto address = asio::ip::make_address(segments[0], ec);
    if (ec) {
      SPDLOG_WARN("asio::ip::make_address failed. Address={}, message={}", segments[0], ec.message());
      state_.store(SessionState::Closing, std::memory_order_relaxed);
      return;
    }

    uint32_t port;
    if (!absl::SimpleAtoi(segments[1], &port)) {
      SPDLOG_WARN("Failed to parse port: {}", segments[1]);
      state_.store(SessionState::Closing, std::memory_order_relaxed);
      return;
    }

    asio::ip::tcp::endpoint endpoint(address, port);
    std::weak_ptr<RemotingSession> session(shared_from_this());

    deadline_.expires_from_now(timeout);
    deadline_.async_wait(std::bind(&RemotingSession::onDeadline, session, std::placeholders::_1));

    socket_->async_connect(endpoint, std::bind(&RemotingSession::onConnection, session, std::placeholders::_1));
  }
}

void RemotingSession::onDeadline(std::weak_ptr<RemotingSession> session, const asio::error_code& ec) {
  auto remoting_session = session.lock();
  if (!remoting_session) {
    return;
  }

  if (ec == asio::error::operation_aborted) {
    SPDLOG_DEBUG("Timer cancelled");
    return;
  }

  SPDLOG_WARN("Deadline triggered");
  remoting_session->socket_->close();
}

void RemotingSession::onConnection(std::weak_ptr<RemotingSession> session, const asio::error_code& ec) {
  std::cout << "RemotingSession::onConnection" << std::endl;

  auto remoting_session = session.lock();

  if (!remoting_session) {
    SPDLOG_WARN("RemotingSession has destructed");
    return;
  }

  if (ec) {
    SPDLOG_WARN("Failed to connect to {}, message: {}", remoting_session->endpoint_, ec.message());
    remoting_session->state_.store(SessionState::Closing, std::memory_order_relaxed);
    return;
  }

  remoting_session->deadline_.cancel();
  SessionState expected = SessionState::Connecting;
  if (remoting_session->state_.compare_exchange_strong(expected, SessionState::Connected)) {
    SPDLOG_INFO("Connection to {} established", remoting_session->endpoint_);
    remoting_session->read_buffer_.resize(1024);
    remoting_session->socket_->async_read_some(
        asio::mutable_buffer(remoting_session->read_buffer_.data(), remoting_session->read_buffer_.size()),
        std::bind(&RemotingSession::onData, session, std::placeholders::_1, std::placeholders::_2));
  }
}

void RemotingSession::onData(std::weak_ptr<RemotingSession> session, const asio::error_code& ec,
                             std::size_t bytes_transferred) {
  auto remoting_session = session.lock();
  if (!remoting_session) {
    SPDLOG_INFO("Remoting session has destructed, dropping {} bytes", bytes_transferred);
    return;
  }

  if (ec) {
    SPDLOG_WARN("Failed to read data from socket. Message: {}", ec.message());
    remoting_session->state_.store(SessionState::Closing, std::memory_order_relaxed);
    return;
  }

  SPDLOG_DEBUG("Received {} bytes from {}", bytes_transferred, remoting_session->endpoint_);
  std::cout << std::string(remoting_session->read_buffer_.data(), bytes_transferred) << std::endl;

  remoting_session->socket_->async_read_some(
      asio::mutable_buffer(remoting_session->read_buffer_.data(), remoting_session->read_buffer_.size()),
      std::bind(&RemotingSession::onData, session, std::placeholders::_1, std::placeholders::_2));
}

void RemotingSession::write(const std::vector<char>& data, std::error_code& ec) {
  if (state_.load(std::memory_order_relaxed) != SessionState::Connected) {
    ec = std::make_error_code(std::errc::not_connected);
    return;
  }

  {
    absl::MutexLock lk(&write_buffer_mtx_);
    write_buffer_.insert(write_buffer_.end(), data.begin(), data.end());
  }

  bool expected = false;
  if (in_flight_flag_.compare_exchange_strong(expected, true)) {
    absl::MutexLock lk(&write_buffer_mtx_);
    in_flight_buffer_.swap(write_buffer_);

    write_buffer_.clear();

    std::weak_ptr<RemotingSession> session(shared_from_this());
    socket_->async_write_some(
        asio::const_buffer(in_flight_buffer_.data(), in_flight_buffer_.size()),
        std::bind(&RemotingSession::onWrite, session, std::placeholders::_1, std::placeholders::_2));
  }
}

void RemotingSession::onWrite(std::weak_ptr<RemotingSession> session, const asio::error_code& ec,
                              std::size_t bytes_transferred) {
  auto remoting_session = session.lock();
  if (!remoting_session) {
    SPDLOG_INFO("RemotingSession has destructed");
    return;
  }

  if (ec) {
    SPDLOG_WARN("Failed to write data to {}. Message: {}", remoting_session->endpoint_, ec.message());
    remoting_session->state_.store(SessionState::Closing, std::memory_order_relaxed);
    remoting_session->socket_->close();
    return;
  }

  auto& in_flight_buffer = remoting_session->in_flight_buffer_;
  in_flight_buffer.erase(in_flight_buffer.begin(), in_flight_buffer.begin() + bytes_transferred);

  bool has_pending_data = true;
  if (in_flight_buffer.empty()) {
    // Swap write and in-flight buffer
    absl::MutexLock lk(&remoting_session->write_buffer_mtx_);
    if (in_flight_buffer.empty()) {
      remoting_session->in_flight_flag_.store(false, std::memory_order_relaxed);
      has_pending_data = false;
    } else {
      in_flight_buffer.swap(remoting_session->write_buffer_);
    }
  }

  if (has_pending_data) {
    remoting_session->socket_->async_write_some(
        asio::const_buffer(in_flight_buffer.data(), in_flight_buffer.size()),
        std::bind(&RemotingSession::onWrite, session, std::placeholders::_1, std::placeholders::_2));
  }
}

ROCKETMQ_NAMESPACE_END