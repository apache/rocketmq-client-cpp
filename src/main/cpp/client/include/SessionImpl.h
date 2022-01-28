#pragma once

#include "Session.h"
#include <apache/rocketmq/v1/definition.pb.h>
#include <apache/rocketmq/v1/service.pb.h>
#include <chrono>
#include <functional>
#include <grpcpp/client_context.h>
#include <memory>
#include <string>

ROCKETMQ_NAMESPACE_BEGIN

class SessionImpl : public Session {
public:
  SessionImpl(std::shared_ptr<grpc::Channel> channel)
      : channel_(std::move(channel)), stub_(rmq::MessagingService::NewStub(channel_)) {
  }

  ~SessionImpl() override = default;

  void queryRoute(absl::flat_hash_map<std::string, std::string> metadata, const rmq::QueryRouteRequest* request,
                  std::function<void(const grpc::Status&, const rmq::QueryRouteResponse&)> callback) override;

  void send(absl::flat_hash_map<std::string, std::string> metadata, const rmq::SendMessageRequest* request,
            std::function<void(const grpc::Status&, const rmq::SendMessageResponse&)> cb) override;

  void queryAssignment(absl::flat_hash_map<std::string, std::string> metadata,
                       const rmq::QueryAssignmentRequest* request,
                       std::function<void(const grpc::Status&, const rmq::QueryAssignmentResponse&)> cb) override;

  void receive(absl::flat_hash_map<std::string, std::string> metadata, const rmq::ReceiveMessageRequest* request,
               std::function<void(const grpc::Status&, const rmq::ReceiveMessageResponse&)> cb) override;

  void ack(absl::flat_hash_map<std::string, std::string> metadata, const rmq::AckMessageRequest* request,
           std::function<void(const grpc::Status&, const rmq::AckMessageResponse&)> cb) override;

private:
  std::shared_ptr<grpc::Channel> channel_;
  std::unique_ptr<rmq::MessagingService::Stub> stub_;
  std::chrono::seconds io_timeout_{3};

  void addMetadata(const absl::flat_hash_map<std::string, std::string>& metadata, grpc::ClientContext* client_context);
  void setDeadline(std::chrono::milliseconds timeout, grpc::ClientContext* client_context);
};

ROCKETMQ_NAMESPACE_END