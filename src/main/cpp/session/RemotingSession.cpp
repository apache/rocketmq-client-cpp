#include "RemotingSession.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <system_error>
#include <vector>

#include "RemotingCommand.h"
#include "absl/memory/memory.h"
#include "absl/strings/numbers.h"
#include "absl/strings/str_split.h"
#include "spdlog/spdlog.h"

ROCKETMQ_NAMESPACE_BEGIN

void RemotingSession::connect(std::chrono::milliseconds timeout, bool await) {
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

    if (await) {
      absl::MutexLock lk(&connect_mtx_);
      connect_cv_.WaitWithTimeout(&connect_mtx_, absl::FromChrono(timeout));
    }
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

void RemotingSession::notifyOnConnection() {
  absl::MutexLock lk(&connect_mtx_);
  connect_cv_.SignalAll();
}

void RemotingSession::onConnection(std::weak_ptr<RemotingSession> session, const asio::error_code& ec) {
  auto remoting_session = session.lock();

  if (!remoting_session) {
    SPDLOG_WARN("RemotingSession has destructed");
    return;
  }

  if (ec) {
    SPDLOG_WARN("Failed to connect to {}, message: {}", remoting_session->endpoint_, ec.message());
    remoting_session->state_.store(SessionState::Closing, std::memory_order_relaxed);
    remoting_session->notifyOnConnection();
    return;
  }

  remoting_session->deadline_.cancel();
  SessionState expected = SessionState::Connecting;
  if (remoting_session->state_.compare_exchange_strong(expected, SessionState::Connected)) {
    SPDLOG_INFO("Connection to {} established", remoting_session->endpoint_);
    remoting_session->read_buffer_.resize(1024);
    remoting_session->fireRead();
  }
  remoting_session->notifyOnConnection();
}

void RemotingSession::fireRead() {
  if (state_.load(std::memory_order_relaxed) != SessionState::Connected) {
    return;
  }

  std::weak_ptr<RemotingSession> session(shared_from_this());
  socket_->async_read_some(
      asio::mutable_buffer(read_buffer_.data() + write_index_, read_buffer_.capacity() - write_index_),
      std::bind(&RemotingSession::onData, session, std::placeholders::_1, std::placeholders::_2));
}

std::vector<RemotingCommand> RemotingSession::fireDecode() {
  std::vector<RemotingCommand> result;
  while (true) {
    if (write_index_ - read_index_ <= 8) {
      SPDLOG_DEBUG("There are {} bytes remaining. Stop decoding", write_index_ - read_index_);
      break;
    }

    std::uint32_t* frame_length_ptr = reinterpret_cast<std::uint32_t*>(read_buffer_.data() + read_index_);
    std::uint32_t* header_length_ptr = reinterpret_cast<std::uint32_t*>(read_buffer_.data() + read_index_ + 4);

    std::uint32_t frame_length = absl::gntohl(*frame_length_ptr);
    std::uint32_t header_length = absl::gntohl(*header_length_ptr);
    if (frame_length + 4 <= write_index_ - read_index_) {
      // A complete frame is available
      std::string json(read_buffer_.data() + read_index_ + 8, header_length);
      google::protobuf::Struct root;
      google::protobuf::util::Status status = google::protobuf::util::JsonStringToMessage(json, &root);
      if (status.ok()) {
        RemotingCommand&& command = RemotingCommand::decode(root);
        RequestCode request_code = RequestCode::QueryRoute;
        {
          absl::MutexLock lk(&opaque_code_mapping_mtx_);
          if (opaque_code_mapping_.contains(command.opaque())) {
            request_code = static_cast<RequestCode>(opaque_code_mapping_[command.opaque()]);
            opaque_code_mapping_.erase(command.opaque());
          }
        }
        const auto& fields = root.fields();
        if (fields.contains("extFields")) {
          command.decodeHeader(request_code, fields.at("extFields"));
        }

        std::size_t body_size = frame_length - header_length - 4;
        if (body_size) {
          command.body_.clear();
          command.body_.resize(body_size);
          memcpy(command.body_.data(), read_buffer_.data() + read_index_ + 8 + header_length, body_size);
        }
        result.emplace_back(std::move(command));
      } else {
        SPDLOG_WARN("Failed to prase JSON. Content: {}, Reason: {}", json,
                    std::string(status.message().data(), status.message().length()));
        state_.store(SessionState::Closing, std::memory_order_relaxed);
        socket_->close();
        break;
      }

      read_index_ += 4 + frame_length;
    } else if (frame_length + 4 > read_buffer_.capacity()) {
      if (frame_length + 4 > MaxFrameLength) {
        // Yuck
        state_.store(SessionState::Closing);
        socket_->close();
        break;
      }

      std::size_t expanded = std::min(2 * frame_length + 8, MaxFrameLength);
      SPDLOG_DEBUG("Expand read_buffer from {} to {} bytes", read_buffer_.capacity(), expanded);
      // We need to expand read_buffer_ to hold the whole frame.
      read_buffer_.resize(expanded);
    }
  }

  if (read_index_) {
    read_buffer_.erase(read_buffer_.begin(), read_buffer_.begin() + read_index_);
    write_index_ -= read_index_;
    read_index_ = 0;
  }

  return result;
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

  remoting_session->write_index_ += bytes_transferred;

  auto&& commands = remoting_session->fireDecode();
  if (!commands.empty()) {
    if (remoting_session->callback_) {
      remoting_session->callback_(commands);
    }
  }

  remoting_session->fireRead();
}

void RemotingSession::write(RemotingCommand command, std::error_code& ec) {
  if (state_.load(std::memory_order_relaxed) != SessionState::Connected) {
    ec = std::make_error_code(std::errc::not_connected);
    return;
  }

  // Maintain opaque-code mapping to help decode
  if (!command.oneWay()) {
    absl::MutexLock lk(&opaque_code_mapping_mtx_);
    opaque_code_mapping_.insert({command.opaque(), command.code()});
  }

  auto&& data = command.encode();

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

  SPDLOG_DEBUG("{} bytes written to {}", bytes_transferred, remoting_session->endpoint_);

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

const std::uint32_t RemotingSession::MaxFrameLength = 16777216;

ROCKETMQ_NAMESPACE_END