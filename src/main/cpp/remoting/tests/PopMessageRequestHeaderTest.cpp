#include "PopMessageRequestHeader.h"
#include "rocketmq/RocketMQ.h"
#include "gtest/gtest.h"

ROCKETMQ_NAMESPACE_BEGIN

TEST(PopMessageRequestHeaderTest, testEncode) {
  PopMessageRequestHeader header;
  google::protobuf::Value root;
  header.encode(root);

  std::string json;
  google::protobuf::util::Status status = google::protobuf::util::MessageToJsonString(root, &json);
  ASSERT_TRUE(status.ok());
}

ROCKETMQ_NAMESPACE_END