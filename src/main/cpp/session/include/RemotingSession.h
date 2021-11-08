#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <system_error>

#include "absl/base/thread_annotations.h"
#include "absl/strings/string_view.h"
#include "absl/synchronization/mutex.h"
#include "asio.hpp"

#include "SessionState.h"

ROCKETMQ_NAMESPACE_BEGIN

class RemotingSession : public std::enable_shared_from_this<RemotingSession> {
public:
  RemotingSession(std::shared_ptr<asio::io_context> context, absl::string_view endpoint)
      : context_(context), endpoint_(endpoint.data(), endpoint.length()), state_(SessionState::Created),
        deadline_(*context) {
  }

  void connect(std::chrono::milliseconds timeout);

  void write(const std::vector<char>& data, std::error_code& ec);

  SessionState state() const {
    return state_.load(std::memory_order_relaxed);
  }

private:
  std::weak_ptr<asio::io_context> context_;
  std::string endpoint_;
  std::unique_ptr<asio::ip::tcp::socket> socket_;
  std::atomic<SessionState> state_;

  asio::steady_timer deadline_;

  std::vector<char> read_buffer_;

  std::atomic_bool in_flight_flag_{false};

  std::vector<char> in_flight_buffer_;

  std::vector<char> write_buffer_ GUARDED_BY(write_buffer_mtx_);
  absl::Mutex write_buffer_mtx_;

  static void onDeadline(std::weak_ptr<RemotingSession> session, const asio::error_code& ec);

  static void onConnection(std::weak_ptr<RemotingSession> session, const asio::error_code& ec);

  static void onData(std::weak_ptr<RemotingSession> session, const asio::error_code& ec, std::size_t bytes_transferred);

  static void onWrite(std::weak_ptr<RemotingSession> session, const asio::error_code& ec,
                      std::size_t bytes_transferred);
};

ROCKETMQ_NAMESPACE_END