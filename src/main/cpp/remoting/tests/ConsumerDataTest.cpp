#include "ConsumerData.h"

#include "ConsumeType.h"
#include "absl/hash/hash_testing.h"
#include "google/protobuf/util/json_util.h"
#include "rocketmq/RocketMQ.h"
#include "gtest/gtest.h"

ROCKETMQ_NAMESPACE_BEGIN

TEST(ConsumerDataTest, testhash) {
  ConsumerData d1;
  ConsumerData d2;
  d2.consume_type_ = ConsumeType::ConsumePassively;

  EXPECT_TRUE(absl::VerifyTypeImplementsAbslHashCorrectly({d1, d2}));
}

TEST(ConsumerDataTest, testEncode) {
  ConsumerData data;
  google::protobuf::Struct root;
  data.encode(root);
  std::string json;
  google::protobuf::util::MessageToJsonString(root, &json);
  std::cout << json << std::endl;
}

ROCKETMQ_NAMESPACE_END