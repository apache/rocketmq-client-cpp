#include <gtest/gtest.h>

#include "AdminServerImpl.h"
#include "apache/rocketmq/v1/admin.grpc.pb.h"

#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/spdlog.h"

#include <thread>

namespace rmq = apache::rocketmq::v1;
using namespace rocketmq::admin;

TEST(AdminServerTest, testSetUp) {

  auto logger = spdlog::basic_logger_mt("rocketmq_logger", "logs/test.log");
  logger->set_level(spdlog::level::debug);
  spdlog::set_default_logger(logger);

  AdminServer* admin_server = new AdminServerImpl;
  admin_server->start();

  std::string address("127.0.0.1:");
  address.append(std::to_string(admin_server->port()));
  auto channel = grpc::CreateChannel(address, grpc::InsecureChannelCredentials());

  auto stub = rmq::Admin::NewStub(channel);

  rmq::ChangeLogLevelRequest request;
  request.set_level(rmq::ChangeLogLevelRequest_Level_INFO);
  rmq::ChangeLogLevelResponse response;

  grpc::ClientContext context;
  context.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(10));

  auto status = stub->ChangeLogLevel(&context, request, &response);

  EXPECT_TRUE(status.ok());
  EXPECT_STREQ("OK", response.remark().c_str());
  EXPECT_EQ(spdlog::level::info, logger->level());

  admin_server->stop();
}