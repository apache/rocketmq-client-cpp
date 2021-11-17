#include "QueryConsumerOffsetRequestHeader.h"
#include "rocketmq/RocketMQ.h"

#include "absl/strings/str_split.h"
#include "google/protobuf/util/json_util.h"
#include "gtest/gtest.h"

ROCKETMQ_NAMESPACE_BEGIN

TEST(QueryConsumerOffsetRequestHeaderTest, testEncode) {
  QueryConsumerOffsetRequestHeader header;
  header.consumer_group_ = "G1";
  header.topic_ = "T1";
  header.queue_id_ = 1;

  google::protobuf::Value root;
  header.encode(root);

  std::string json;
  google::protobuf::util::MessageToJsonString(root, &json);
  ASSERT_TRUE(absl::StrContains(json, header.consumer_group_));
  ASSERT_TRUE(absl::StrContains(json, header.topic_));
}

ROCKETMQ_NAMESPACE_END