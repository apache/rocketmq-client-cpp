#pragma once
#include "apache/rocketmq/v1/admin.grpc.pb.h"
#include "rocketmq/RocketMQ.h"
#include <grpcpp/grpcpp.h>
#include <memory>

namespace rmq = apache::rocketmq::v1;

ROCKETMQ_NAMESPACE_BEGIN

using grpc::Channel;
using grpc::Status;

namespace admin {

class AdminClient {
public:
  explicit AdminClient(std::shared_ptr<Channel>& channel) : stub_(rmq::Admin::NewStub(channel)) {}

  Status changeLogLevel(const rmq::ChangeLogLevelRequest& request, rmq::ChangeLogLevelResponse& response);

private:
  std::unique_ptr<rmq::Admin::Stub> stub_;
};

} // namespace admin

ROCKETMQ_NAMESPACE_END