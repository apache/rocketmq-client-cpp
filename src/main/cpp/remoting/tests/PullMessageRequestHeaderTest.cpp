#include "gtest/gtest.h"
#include "google/protobuf/util/json_util.h"

#include "PullMessageRequestHeader.h"
#include "rocketmq/RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

TEST(PullMessageRequestHeaderTest, testEncode) {
  PullMessageRequestHeader header;
  google::protobuf::Value root;
  header.encode(root);

  std::string json;
  google::protobuf::util::MessageToJsonString(root, &json);
  std::cout << json << std::endl;
}

ROCKETMQ_NAMESPACE_END