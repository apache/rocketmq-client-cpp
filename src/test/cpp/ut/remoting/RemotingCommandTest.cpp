#include "RemotingCommand.h"

#include <cstdint>
#include <iostream>
#include <unordered_set>

#include "QueryRouteRequestHeader.h"
#include "gtest/gtest.h"

ROCKETMQ_NAMESPACE_BEGIN

class RemotingCommandTest : public testing::Test {
public:
};

TEST_F(RemotingCommandTest, testNextRequestId) {

  std::unordered_set<std::int32_t> request_ids;

  for (int i = 0; i < 50; i++) {
    std::int32_t id = RemotingCommand::nextRequestId();
    EXPECT_TRUE(request_ids.find(id) == request_ids.end());
    request_ids.insert(id);
  }
}

TEST_F(RemotingCommandTest, testCreateRequest) {
  auto header = new QueryRouteRequestHeader;
  header->topic("abc");

  auto command = RemotingCommand::createRequest(RequestCode::QueryRoute, header);
  google::protobuf::Value root;
  command.encodeHeader(root);

  const auto& fields = root.struct_value().fields();
  EXPECT_TRUE(fields.contains("extFields"));
  EXPECT_TRUE(fields.contains("code"));
  EXPECT_EQ(static_cast<std::int32_t>(RequestCode::QueryRoute), fields.at("code").number_value());

  std::string json;
  auto status = google::protobuf::util::MessageToJsonString(root, &json);
  EXPECT_TRUE(status.ok());
  std::cout << json << std::endl;
}

ROCKETMQ_NAMESPACE_END