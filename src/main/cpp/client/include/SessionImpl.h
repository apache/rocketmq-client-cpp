#pragma once

#include "Session.h"
#include <apache/rocketmq/v1/definition.pb.h>
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

private:
  std::shared_ptr<grpc::Channel> channel_;
  std::unique_ptr<rmq::MessagingService::Stub> stub_;
};

ROCKETMQ_NAMESPACE_END