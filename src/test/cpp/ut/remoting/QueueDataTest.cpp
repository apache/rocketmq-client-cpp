#include "gtest/gtest.h"

#include "QueueData.h"
#include "rocketmq/RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

TEST(QueueDataTest, testDecode) {
  std::string json = R"({"brokerName":"broker1","perm":6,"readQueueNums":4,"topicSynFlag":3,"writeQueueNums":8})";
  google::protobuf::Struct root;
  auto status = google::protobuf::util::JsonStringToMessage(json, &root);
  EXPECT_TRUE(status.ok());

  QueueData queue_data = QueueData::decode(root);
  EXPECT_EQ("broker1", queue_data.broker_name_);
  EXPECT_EQ(6, queue_data.perm_);
  EXPECT_EQ(4, queue_data.read_queue_number_);
  EXPECT_EQ(8, queue_data.write_queue_number_);
  EXPECT_EQ(3, queue_data.topic_system_flag_);
}

ROCKETMQ_NAMESPACE_END