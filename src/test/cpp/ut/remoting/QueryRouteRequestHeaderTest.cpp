#include "QueryRouteRequestHeader.h"

#include "gtest/gtest.h"

ROCKETMQ_NAMESPACE_BEGIN

TEST(QueryRouteRequestHeaderTest, testEncode) {
  google::protobuf::Value root;
  QueryRouteRequestHeader header;
  header.topic("abc");
  header.encode(root);

  std::string json;
  auto status = google::protobuf::util::MessageToJsonString(root, &json);
  EXPECT_TRUE(status.ok());
  EXPECT_FALSE(json.empty());
  std::cout << json << std::endl;
}

ROCKETMQ_NAMESPACE_END