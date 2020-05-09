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
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>

#include "DataBlock.h"
#include "TopicRouteData.h"

using testing::InitGoogleMock;
using testing::InitGoogleTest;
using testing::Return;

using rocketmq::BrokerData;
using rocketmq::MemoryBlock;
using rocketmq::QueueData;
using rocketmq::TopicRouteData;

TEST(TopicRouteDataTest, TopicRouteData) {
  Json::Value root;
  root["orderTopicConf"] = "orderTopicConf";

  Json::Value queueData;
  queueData["brokerName"] = "brokerTest";
  queueData["readQueueNums"] = 8;
  queueData["writeQueueNums"] = 8;
  queueData["perm"] = 7;

  Json::Value queueDatas;
  queueDatas[0] = queueData;

  root["queueDatas"] = queueDatas;

  Json::Value brokerData;
  brokerData["brokerName"] = "testBroker";

  Json::Value brokerAddrs;
  brokerAddrs["0"] = "127.0.0.1:10091";
  brokerAddrs["1"] = "127.0.0.2:10092";
  brokerData["brokerAddrs"] = brokerAddrs;

  Json::Value brokerDatas;
  brokerDatas[0] = brokerData;

  root["brokerDatas"] = brokerDatas;

  std::string out = root.toStyledString();

  std::unique_ptr<MemoryBlock> block(new MemoryBlock(const_cast<char*>(out.data()), out.size()));
  std::unique_ptr<TopicRouteData> topicRouteData(TopicRouteData::Decode(*block));

  EXPECT_EQ(root["orderTopicConf"], topicRouteData->getOrderTopicConf());

  BrokerData broker;
  broker.brokerName = "testBroker";
  broker.brokerAddrs[0] = "127.0.0.1:10091";
  broker.brokerAddrs[1] = "127.0.0.2:10092";

  std::vector<BrokerData> brokerDataSt = topicRouteData->getBrokerDatas();
  EXPECT_EQ(broker, brokerDataSt[0]);

  QueueData queue;
  queue.brokerName = "brokerTest";
  queue.readQueueNums = 8;
  queue.writeQueueNums = 8;
  queue.perm = 7;
  std::vector<QueueData> queueDataSt = topicRouteData->getQueueDatas();
  EXPECT_EQ(queue, queueDataSt[0]);

  EXPECT_EQ(topicRouteData->selectBrokerAddr(), "127.0.0.1:10091");
}

int main(int argc, char* argv[]) {
  InitGoogleMock(&argc, argv);
  testing::GTEST_FLAG(throw_on_failure) = true;
  testing::GTEST_FLAG(filter) = "TopicRouteDataTest.*";
  return RUN_ALL_TESTS();
}
