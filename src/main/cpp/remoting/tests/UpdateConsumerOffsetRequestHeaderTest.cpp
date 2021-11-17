#include "UpdateConsumerOffsetRequestHeader.h"
#include "rocketmq/RocketMQ.h"

#include "gtest/gtest.h"

ROCKETMQ_NAMESPACE_BEGIN

TEST(UpdateConsumerOffsetRequestHeaderTest, testEncode) {
  UpdateConsumerOffsetRequestHeader header;
  header.consumer_group_ = "G1";
  header.topic_ = "T1";
  header.commit_offset_ = 3;

  google::protobuf::Value root;
  header.encode(root);

  std::string json;
  google::protobuf::util::MessageToJsonString(root, &json);
  std::cout << json << std::endl;
}

ROCKETMQ_NAMESPACE_END