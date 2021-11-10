#include "QueueData.h"
#include "rocketmq/RocketMQ.h"

#include "gtest/gtest.h"

ROCKETMQ_NAMESPACE_BEGIN

TEST(QueueDataTest, testDecode) {
  std::string json = R"({"brokerName":"broker-a","perm":6,"writeQueueNums":8,"readQueueNums":8,"topicSynFlag":0})";
  google::protobuf::Struct root;
  auto status = google::protobuf::util::JsonStringToMessage(json, &root);
  ASSERT_TRUE(status.ok());

  auto&& queue_data = QueueData::decode(root);
  ASSERT_EQ("broker-a", queue_data.broker_name_);
  ASSERT_EQ(6, queue_data.perm_);
  ASSERT_EQ(8, queue_data.write_queue_number_);
  ASSERT_EQ(8, queue_data.read_queue_number_);
  ASSERT_EQ(0, queue_data.topic_system_flag_);
}

ROCKETMQ_NAMESPACE_END