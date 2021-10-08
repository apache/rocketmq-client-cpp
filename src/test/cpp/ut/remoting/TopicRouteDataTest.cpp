#include "TopicRouteData.h"
#include "rocketmq/RocketMQ.h"
#include "gtest/gtest.h"

ROCKETMQ_NAMESPACE_BEGIN

TEST(TopicRouteDataTest, testDecode) {
  std::string json =
      R"({"brokerDatas":[{"brokerAddrs":{"1":"abc","2":"def"},"brokerName":"b1","cluster":"cluster","enableActingMaster":false},{"brokerAddrs":{"1":"abc","2":"def"},"brokerName":"b1","cluster":"cluster","enableActingMaster":false},{"brokerAddrs":{"1":"abc","2":"def"},"brokerName":"b1","cluster":"cluster","enableActingMaster":false},{"brokerAddrs":{"1":"abc","2":"def"},"brokerName":"b1","cluster":"cluster","enableActingMaster":false},{"brokerAddrs":{"1":"abc","2":"def"},"brokerName":"b1","cluster":"cluster","enableActingMaster":false},{"brokerAddrs":{"1":"abc","2":"def"},"brokerName":"b1","cluster":"cluster","enableActingMaster":false},{"brokerAddrs":{"1":"abc","2":"def"},"brokerName":"b1","cluster":"cluster","enableActingMaster":false},{"brokerAddrs":{"1":"abc","2":"def"},"brokerName":"b1","cluster":"cluster","enableActingMaster":false}],"filterServerTable":{},"queueDatas":[{"brokerName":"broker1","perm":6,"readQueueNums":4,"topicSynFlag":3,"writeQueueNums":8},{"brokerName":"broker1","perm":6,"readQueueNums":4,"topicSynFlag":3,"writeQueueNums":8},{"brokerName":"broker1","perm":6,"readQueueNums":4,"topicSynFlag":3,"writeQueueNums":8},{"brokerName":"broker1","perm":6,"readQueueNums":4,"topicSynFlag":3,"writeQueueNums":8},{"brokerName":"broker1","perm":6,"readQueueNums":4,"topicSynFlag":3,"writeQueueNums":8},{"brokerName":"broker1","perm":6,"readQueueNums":4,"topicSynFlag":3,"writeQueueNums":8},{"brokerName":"broker1","perm":6,"readQueueNums":4,"topicSynFlag":3,"writeQueueNums":8},{"brokerName":"broker1","perm":6,"readQueueNums":4,"topicSynFlag":3,"writeQueueNums":8}]})";
  google::protobuf::Struct root;
  auto status = google::protobuf::util::JsonStringToMessage(json, &root);
  EXPECT_TRUE(status.ok());

  auto topic_route_data = TopicRouteData::decode(root);

  EXPECT_FALSE(topic_route_data.broker_data_.empty());
  EXPECT_FALSE(topic_route_data.queue_data_.empty());
}

ROCKETMQ_NAMESPACE_END