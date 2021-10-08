#include "BrokerData.h"

#include <iostream>

#include "rocketmq/RocketMQ.h"
#include "gtest/gtest.h"

#include "google/protobuf/struct.pb.h"

ROCKETMQ_NAMESPACE_BEGIN

TEST(BrokerDataTest, testDecode) {
  std::string json =
      R"({"brokerAddrs":{"1":"abc","2":"def"},"brokerName":"b1","cluster":"cluster","enableActingMaster":false})";
  google::protobuf::Struct root;

  auto status = google::protobuf::util::JsonStringToMessage(json, &root);
  ASSERT_TRUE(status.ok());

  BrokerData&& broker_data = BrokerData::decode(root);
  EXPECT_EQ("b1", broker_data.broker_name_);
  EXPECT_EQ("cluster", broker_data.cluster_);
  EXPECT_EQ(2, broker_data.broker_addresses_.size());
  EXPECT_EQ("abc", broker_data.broker_addresses_.at(1));
  EXPECT_EQ("def", broker_data.broker_addresses_.at(2));
}

ROCKETMQ_NAMESPACE_END