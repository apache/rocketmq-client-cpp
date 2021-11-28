#pragma once

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <system_error>

#include "absl/base/thread_annotations.h"
#include "absl/container/flat_hash_map.h"
#include "absl/strings/string_view.h"
#include "absl/synchronization/mutex.h"
#include "asio.hpp"

#include "RemotingCommand.h"
#include "SessionState.h"

ROCKETMQ_NAMESPACE_BEGIN

class RemotingSession : public std::enable_shared_from_this<RemotingSession> {
public:
  RemotingSession(std::shared_ptr<asio::io_context> context, absl::string_view endpoint,
                  std::function<void(const std::vector<RemotingCommand>&)> callback)
      : context_(context), endpoint_(endpoint.data(), endpoint.length()), state_(SessionState::Created),
        deadline_(*context), callback_(callback) {
    
  }

  void connect(std::chrono::milliseconds timeout, bool await = false);

  void write(RemotingCommand command, std::error_code& ec);

  SessionState state() const {
    return state_.load(std::memory_order_relaxed);
  }

  void onDisconnect(const std::error_code& ec) LOCKS_EXCLUDED(opaque_code_mapping_mtx_);

private:
  std::weak_ptr<asio::io_context> context_;
  std::string endpoint_;
  std::unique_ptr<asio::ip::tcp::socket> socket_;
  std::atomic<SessionState> state_;

  absl::Mutex connect_mtx_;
  absl::CondVar connect_cv_;
  asio::steady_timer deadline_;

  std::vector<char> read_buffer_;
  std::size_t read_index_{0};
  std::size_t write_index_{0};

  std::atomic_bool in_flight_flag_{false};

  std::vector<char> in_flight_buffer_;

  std::vector<char> write_buffer_ GUARDED_BY(write_buffer_mtx_);
  absl::Mutex write_buffer_mtx_;

  /**
   * @brief
   *  TODO: When session is disrupted, we need to respond all in-flight requests with errors.
   */
  absl::flat_hash_map<std::int32_t, std::int32_t> opaque_code_mapping_ GUARDED_BY(opaque_code_mapping_mtx_);
  absl::Mutex opaque_code_mapping_mtx_;

  std::function<void(const std::vector<RemotingCommand>&)> callback_;

  void notifyOnConnection();

  static void onDeadline(std::weak_ptr<RemotingSession> session, const asio::error_code& ec);

  static void onConnection(std::weak_ptr<RemotingSession> session, const asio::error_code& ec);

  static void onData(std::weak_ptr<RemotingSession> session, const asio::error_code& ec, std::size_t bytes_transferred);

  std::vector<RemotingCommand> fireDecode();

  void fireRead();

  static void onWrite(std::weak_ptr<RemotingSession> session, const asio::error_code& ec,
                      std::size_t bytes_transferred);

  static const std::uint32_t MaxFrameLength;
};

ROCKETMQ_NAMESPACE_END