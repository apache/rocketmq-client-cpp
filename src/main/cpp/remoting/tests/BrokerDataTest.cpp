#include "BrokerData.h"
#include "rocketmq/RocketMQ.h"

#include "gtest/gtest.h"

ROCKETMQ_NAMESPACE_BEGIN

TEST(BrokerDataTest, testDecode) {
  std::string json = R"(
{"brokerName":"broker-a","brokerAddrs":{"0":"11.163.70.118:10911"},"cluster":"DefaultCluster","enableActingMaster":false}
  )";
  google::protobuf::Struct root;
  auto status = google::protobuf::util::JsonStringToMessage(json, &root);
  ASSERT_TRUE(status.ok());
  auto&& broker_data = BrokerData::decode(root);
  ASSERT_EQ("broker-a", broker_data.broker_name_);
  ASSERT_EQ("DefaultCluster", broker_data.cluster_);
  ASSERT_EQ(1, broker_data.broker_addresses_.size());
}

ROCKETMQ_NAMESPACE_END