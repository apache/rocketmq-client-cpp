#include "HeartbeatData.h"
#include "ConsumeType.h"
#include "SubscriptionData.h"
#include "google/protobuf/util/json_util.h"
#include "rocketmq/RocketMQ.h"
#include "gtest/gtest.h"

ROCKETMQ_NAMESPACE_BEGIN

TEST(HeartbeatDataTest, testEncode) {
  HeartbeatData data;
  data.client_id_ = "C0";

  ConsumerData consumer;

  {
    SubscriptionData sub;
    sub.class_filter_mode_ = true;
    sub.topic_ = "T1";
    consumer.subscription_data_set_.emplace(sub);
  }

  data.consumer_data_set_.emplace(consumer);

  google::protobuf::Struct root;
  data.encode(root);

  std::string json;
  google::protobuf::util::MessageToJsonString(root, &json);
  std::cout << json << std::endl;
}

ROCKETMQ_NAMESPACE_END
