#pragma once
#include "apache/rocketmq/v1/admin.grpc.pb.h"
#include "rocketmq/RocketMQ.h"
#include <grpcpp/grpcpp.h>

using grpc::ServerContext;
using grpc::Status;
namespace rmq = apache::rocketmq::v1;

ROCKETMQ_NAMESPACE_BEGIN

namespace admin {
class AdminServiceImpl final : public rmq::Admin::Service {
public:
  Status ChangeLogLevel(ServerContext* context, const rmq::ChangeLogLevelRequest* request,
                        rmq::ChangeLogLevelResponse* reply) override;
};
} // namespace admin

ROCKETMQ_NAMESPACE_END