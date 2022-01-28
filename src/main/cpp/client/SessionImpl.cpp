#include "SessionImpl.h"

#include <chrono>
#include <grpcpp/client_context.h>
#include <string>

#include "absl/memory/memory.h"
#include "apache/rocketmq/v1/service.pb.h"
#include "grpcpp/client_context.h"

ROCKETMQ_NAMESPACE_BEGIN

void SessionImpl::queryRoute(absl::flat_hash_map<std::string, std::string> metadata,
                             const rmq::QueryRouteRequest* request,
                             std::function<void(const grpc::Status&, const rmq::QueryRouteResponse&)> cb) {
  auto response = new rmq::QueryRouteResponse;
  auto client_context = new grpc::ClientContext;

  addMetadata(metadata, client_context);
  setDeadline(io_timeout_, client_context);

  auto callback = [=](grpc::Status s) {
    auto reply = absl::WrapUnique(response);
    auto ctx = absl::WrapUnique(client_context);
    cb(s, *response);
  };

  stub_->async()->QueryRoute(client_context, request, response, callback);
}

void SessionImpl::send(absl::flat_hash_map<std::string, std::string> metadata, const rmq::SendMessageRequest* request,
                       std::function<void(const grpc::Status&, const rmq::SendMessageResponse&)> cb) {
  auto response = new rmq::SendMessageResponse;
  auto client_context = new grpc::ClientContext;

  setDeadline(io_timeout_, client_context);
  addMetadata(metadata, client_context);

  auto callback = [=](grpc::Status s) {
    auto reply = absl::WrapUnique(response);
    auto ctx = absl::WrapUnique(client_context);
    cb(s, *reply);
  };

  stub_->async()->SendMessage(client_context, request, response, callback);
}

void SessionImpl::queryAssignment(absl::flat_hash_map<std::string, std::string> metadata,
                                  const rmq::QueryAssignmentRequest* request,
                                  std::function<void(const grpc::Status&, const rmq::QueryAssignmentResponse&)> cb) {
  auto response = new rmq::QueryAssignmentResponse;
  auto client_context = new grpc::ClientContext;
  addMetadata(metadata, client_context);
  setDeadline(io_timeout_, client_context);
  auto callback = [=](grpc::Status status) {
    auto reply = absl::WrapUnique(response);
    auto ctx = absl::WrapUnique(client_context);
    cb(status, *reply);
  };
  stub_->async()->QueryAssignment(client_context, request, response, callback);
}

void SessionImpl::receive(absl::flat_hash_map<std::string, std::string> metadata,
                          const rmq::ReceiveMessageRequest* request,
                          std::function<void(const grpc::Status&, const rmq::ReceiveMessageResponse&)> cb) {
  auto response = new rmq::ReceiveMessageResponse;
  auto client_context = new grpc::ClientContext;
  addMetadata(metadata, client_context);
  setDeadline(io_timeout_, client_context);
  auto callback = [=](grpc::Status status) {
    auto reply = absl::WrapUnique(response);
    auto ctx = absl::WrapUnique(client_context);
    cb(status, *reply);
  };
  stub_->async()->ReceiveMessage(client_context, request, response, callback);
}

void SessionImpl::ack(absl::flat_hash_map<std::string, std::string> metadata, const rmq::AckMessageRequest* request,
                      std::function<void(const grpc::Status&, const rmq::AckMessageResponse&)> cb) {
  auto response = new rmq::AckMessageResponse;
  auto client_context = new grpc::ClientContext;
  addMetadata(metadata, client_context);
  setDeadline(io_timeout_, client_context);
  auto callback = [=](grpc::Status status) {
    auto reply = absl::WrapUnique(response);
    auto ctx = absl::WrapUnique(client_context);
    cb(status, *reply);
  };
  stub_->async()->AckMessage(client_context, request, response, callback);
}

void SessionImpl::heartbeat(absl::flat_hash_map<std::string, std::string> metadata,
                            const rmq::HeartbeatRequest* request,
                            std::function<void(const grpc::Status&, const rmq::HeartbeatResponse&)> cb) {
  auto response = new rmq::HeartbeatResponse;
  auto client_context = new grpc::ClientContext;
  addMetadata(metadata, client_context);
  setDeadline(io_timeout_, client_context);
  auto callback = [=](grpc::Status status) {
    auto reply = absl::WrapUnique(response);
    auto ctx = absl::WrapUnique(client_context);
    cb(status, *reply);
  };
  stub_->async()->Heartbeat(client_context, request, response, callback);
}

void SessionImpl::healthCheck(absl::flat_hash_map<std::string, std::string> metadata,
                              const rmq::HealthCheckRequest* request,
                              std::function<void(const grpc::Status&, const rmq::HealthCheckResponse&)> cb) {
  auto response = new rmq::HealthCheckResponse;
  auto client_context = new grpc::ClientContext;
  addMetadata(metadata, client_context);
  setDeadline(io_timeout_, client_context);
  auto callback = [=](grpc::Status status) {
    auto reply = absl::WrapUnique(response);
    auto ctx = absl::WrapUnique(client_context);
    cb(status, *reply);
  };
  stub_->async()->HealthCheck(client_context, request, response, callback);
}

void SessionImpl::endTransaction(absl::flat_hash_map<std::string, std::string> metadata,
                                 const rmq::EndTransactionRequest* request,
                                 std::function<void(const grpc::Status&, const rmq::EndTransactionResponse&)> cb) {
  auto response = new rmq::EndTransactionResponse;
  auto client_context = new grpc::ClientContext;
  addMetadata(metadata, client_context);
  setDeadline(io_timeout_, client_context);
  auto callback = [=](grpc::Status status) {
    auto reply = absl::WrapUnique(response);
    auto ctx = absl::WrapUnique(client_context);
    cb(status, *reply);
  };
  stub_->async()->EndTransaction(client_context, request, response, callback);
}

void SessionImpl::queryOffset(absl::flat_hash_map<std::string, std::string> metadata,
                              const rmq::QueryOffsetRequest* request,
                              std::function<void(const grpc::Status&, const rmq::QueryOffsetResponse&)> cb) {
  auto response = new rmq::QueryOffsetResponse;
  auto client_context = new grpc::ClientContext;
  addMetadata(metadata, client_context);
  setDeadline(io_timeout_, client_context);
  auto callback = [=](grpc::Status status) {
    auto reply = absl::WrapUnique(response);
    auto ctx = absl::WrapUnique(client_context);
    cb(status, *reply);
  };
  stub_->async()->QueryOffset(client_context, request, response, callback);
}

void SessionImpl::pull(absl::flat_hash_map<std::string, std::string> metadata, const rmq::PullMessageRequest* request,
                       std::function<void(const grpc::Status&, const rmq::PullMessageResponse&)> cb) {
  auto response = new rmq::PullMessageResponse;
  auto client_context = new grpc::ClientContext;
  addMetadata(metadata, client_context);
  setDeadline(io_timeout_, client_context);
  auto callback = [=](grpc::Status status) {
    auto reply = absl::WrapUnique(response);
    auto ctx = absl::WrapUnique(client_context);
    cb(status, *reply);
  };
  stub_->async()->PullMessage(client_context, request, response, callback);
}

void SessionImpl::forwardMessageToDeadLetterQueue(
    absl::flat_hash_map<std::string, std::string> metadata, const rmq::ForwardMessageToDeadLetterQueueRequest* request,
    std::function<void(const grpc::Status&, const rmq::ForwardMessageToDeadLetterQueueResponse&)> cb) {
  auto response = new rmq::ForwardMessageToDeadLetterQueueResponse;
  auto client_context = new grpc::ClientContext;
  addMetadata(metadata, client_context);
  setDeadline(io_timeout_, client_context);
  auto callback = [=](grpc::Status status) {
    auto reply = absl::WrapUnique(response);
    auto ctx = absl::WrapUnique(client_context);
    cb(status, *reply);
  };
  stub_->async()->ForwardMessageToDeadLetterQueue(client_context, request, response, callback);
}

void SessionImpl::addMetadata(const absl::flat_hash_map<std::string, std::string>& metadata,
                              grpc::ClientContext* client_context) {
  for (const auto& entry : metadata) {
    client_context->AddMetadata(entry.first, entry.second);
  }
}

void SessionImpl::setDeadline(std::chrono::milliseconds timeout, grpc::ClientContext* client_context) {
  client_context->set_deadline(std::chrono::system_clock::now() + io_timeout_);
}

ROCKETMQ_NAMESPACE_END