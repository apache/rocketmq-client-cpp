/*
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
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