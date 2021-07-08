#pragma once

#include "LoggerImpl.h"
#include "absl/synchronization/mutex.h"
#include "rocketmq/RocketMQ.h"
#include <atomic>
#include <ctime>
#include <functional>
#include <grpcpp/client_context.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/impl/codegen/async_stream.h>
#include <grpcpp/impl/codegen/async_unary_call.h>

ROCKETMQ_NAMESPACE_BEGIN

/**
 * Note:
 *   Before modifying anything in this file, ensure you have read all comments in completion_queue_impl.h and
 *   async_stream.h
 */
struct BaseInvocationContext {
  virtual ~BaseInvocationContext() = default;
  virtual void onCompletion(bool ok) = 0;

  grpc::ClientContext context_;
  grpc::Status status_;
  std::chrono::system_clock::time_point created_time_{std::chrono::system_clock::now()};
  std::chrono::steady_clock::time_point start_time_{std::chrono::steady_clock::now()};
};

template <typename T>
struct InvocationContext : public BaseInvocationContext {

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

      if (callback_) {
        callback_(grpc::Status::CANCELLED, context_, response_);
      }

      delete this;
      return;
    }

    if (!status_.ok() && grpc::StatusCode::DEADLINE_EXCEEDED == status_.error_code()) {
      std::time_t creation_time_t = std::chrono::system_clock::to_time_t(created_time_);
      auto fraction =
          std::chrono::duration_cast<std::chrono::milliseconds>(created_time_.time_since_epoch()).count() % 1000;
      char fmt_date_time[128];

      /**
       * TODO: std::localtime is not thread-safe, output, as a result, may be less reliable in highly contending
       * scenario
       */
      std::strftime(fmt_date_time, sizeof(fmt_date_time), "%Y-%m-%d %H:%M:%S", std::localtime(&creation_time_t));

      auto elapsed =
          std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_time_).count();
      auto diff =
          std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - context_.deadline())
              .count();
      SPDLOG_WARN("Asynchronous RPC[{}.{}] timed out, elapsing {}ms, deadline-over-due: {}ms", fmt_date_time, fraction,
                  elapsed, diff);
    }
    try {
      if (callback_) {
        callback_(status_, context_, response_);
      }
    } catch (const std::exception& e) {
      SPDLOG_WARN("Unexpected error while invoking user-defined callback. Reason: {}", e.what());
    } catch (...) {
      SPDLOG_WARN("Unexpected error while invoking user-defined callback");
    }
    delete this;
  }

  T response_;
  std::function<void(const grpc::Status&, const grpc::ClientContext&, const T&)> callback_;
  std::unique_ptr<grpc::ClientAsyncResponseReader<T>> response_reader_;
};
ROCKETMQ_NAMESPACE_END