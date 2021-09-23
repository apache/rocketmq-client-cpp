#pragma once

#include <atomic>
#include <ctime>
#include <functional>

#include "absl/synchronization/mutex.h"
#include "absl/time/time.h"
#include "grpcpp/client_context.h"
#include "grpcpp/grpcpp.h"
#include "grpcpp/impl/codegen/async_stream.h"
#include "grpcpp/impl/codegen/async_unary_call.h"

#include "LoggerImpl.h"
#include "MetadataConstants.h"
#include "UniqueIdGenerator.h"
#include "rocketmq/RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

/**
 * Note:
 *   Before modifying anything in this file, ensure you have read all comments in completion_queue_impl.h and
 *   async_stream.h
 */
struct BaseInvocationContext {
  BaseInvocationContext() : request_id_(UniqueIdGenerator::instance().next()) {
    context.AddMetadata(MetadataConstants::REQUEST_ID_KEY, request_id_);
  }

  virtual ~BaseInvocationContext() = default;
  virtual void onCompletion(bool ok) = 0;
  std::string request_id_;
  std::string remote_address;
  grpc::ClientContext context;
  grpc::Status status;
  absl::Time created_time{absl::Now()};
  std::chrono::steady_clock::time_point start_time{std::chrono::steady_clock::now()};
};

template <typename T> struct InvocationContext : public BaseInvocationContext {

  void onCompletion(bool ok) override {
    /// Client-side Read, Server-side Read, Client-side
    /// RecvInitialMetadata (which is typically included in Read if not
    /// done explicitly): ok indicates whether there is a valid message
    /// that got read. If not, you know that there are certainly no more
    /// messages that can ever be read from this stream. For the client-side
    /// operations, this only happens because the call is dead. For the
    /// server-side operation, though, this could happen because the client
    /// has done a WritesDone already.
    if (!ok) {
      SPDLOG_WARN("One async call is already dead");
      if (callback) {
        callback(this);
      }
      delete this;
      return;
    }

    if (!status.ok() && grpc::StatusCode::DEADLINE_EXCEEDED == status.error_code()) {
      auto elapsed =
          std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_time).count();
      auto diff =
          std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - context.deadline())
              .count();
      SPDLOG_WARN("Asynchronous RPC[{}.{}] timed out, elapsing {}ms, deadline-over-due: {}ms",
                  absl::FormatTime(created_time, absl::UTCTimeZone()), elapsed, diff);
    }
    try {
      if (callback) {
        callback(this);
      }
    } catch (const std::exception& e) {
      SPDLOG_WARN("Unexpected error while invoking user-defined callback. Reason: {}", e.what());
    } catch (...) {
      SPDLOG_WARN("Unexpected error while invoking user-defined callback");
    }
    delete this;
  }

  T response;
  std::function<void(const InvocationContext<T>*)> callback;
  std::unique_ptr<grpc::ClientAsyncResponseReader<T>> response_reader;
};
ROCKETMQ_NAMESPACE_END