#include "AdminServiceImpl.h"
#include "spdlog/spdlog.h"

ROCKETMQ_NAMESPACE_BEGIN

namespace admin {

Status applyChange(const rmq::ChangeLogLevelRequest* request) {
  auto logger = spdlog::get("rocketmq_logger");
  if (!logger) {
    return Status(grpc::StatusCode::INTERNAL, "rocketmq_logger is not registered");
  }

  auto level = request->level();
  switch (level) {
    case rmq::ChangeLogLevelRequest_Level_TRACE:
      logger->set_level(spdlog::level::trace);
      break;
    case rmq::ChangeLogLevelRequest_Level_DEBUG:
      logger->set_level(spdlog::level::debug);
      break;
    case rmq::ChangeLogLevelRequest_Level_INFO:
      logger->set_level(spdlog::level::info);
      break;
    case rmq::ChangeLogLevelRequest_Level_WARN:
      logger->set_level(spdlog::level::warn);
      break;
    case rmq::ChangeLogLevelRequest_Level_ERROR:
      logger->set_level(spdlog::level::err);
      break;
    default:
      logger->set_level(spdlog::level::info);
      break;
  }
  return grpc::Status::OK;
}

Status AdminServiceImpl::ChangeLogLevel(ServerContext* context, const rmq::ChangeLogLevelRequest* request,
                                        rmq::ChangeLogLevelResponse* reply) {
  Status status = applyChange(request);
  if (status.ok()) {
    reply->set_remark("OK");
  }
  return status;
}

} // namespace admin

ROCKETMQ_NAMESPACE_END