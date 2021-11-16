#include "gtest/gtest.h"

#include "absl/hash/hash_testing.h"
#include "google/protobuf/util/json_util.h"

#include "SubscriptionData.h"
#include "rocketmq/RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

TEST(SubscriptionDataTest, testHash) {
  SubscriptionData s1;
  SubscriptionData s2;
  s2.class_filter_mode_ = true;
  EXPECT_TRUE(absl::VerifyTypeImplementsAbslHashCorrectly({s1, s2}));
}

TEST(SubscriptionDataTest, testEncode) {
  SubscriptionData subscription_data;
  subscription_data.class_filter_mode_ = true;
  subscription_data.topic_ = "Topic-0";
  subscription_data.sub_string_ = "*";
  subscription_data.sub_version_ = 123;

  google::protobuf::Struct root;
  subscription_data.encode(root);

  std::string json;
  google::protobuf::util::MessageToJsonString(root, &json);
  std::cout << json << std::endl;
}

ROCKETMQ_NAMESPACE_END
