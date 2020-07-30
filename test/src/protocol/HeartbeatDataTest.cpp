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

#include <vector>

#include "protocol/heartbeat/HeartbeatData.hpp"

using testing::InitGoogleMock;
using testing::InitGoogleTest;
using testing::Return;

using rocketmq::ConsumeFromWhere;
using rocketmq::ConsumerData;
using rocketmq::ConsumeType;
using rocketmq::HeartbeatData;
using rocketmq::MessageModel;
using rocketmq::ProducerData;
using rocketmq::SubscriptionData;

TEST(HeartbeatDataTest, ProducerData) {
  ProducerData producerData("testGroup");

  Json::Value outJson = producerData.toJson();
  EXPECT_EQ(outJson["groupName"], "testGroup");
}

TEST(HeartbeatDataTest, ConsumerData) {
  ConsumerData consumerData("testGroup", ConsumeType::CONSUME_ACTIVELY, MessageModel::BROADCASTING,
                            ConsumeFromWhere::CONSUME_FROM_TIMESTAMP,
                            std::vector<SubscriptionData>{SubscriptionData("testTopic", "sub")});

  Json::Value outJson = consumerData.toJson();

  EXPECT_EQ(outJson["groupName"], "testGroup");

  EXPECT_EQ(outJson["consumeType"].asInt(), ConsumeType::CONSUME_ACTIVELY);
  EXPECT_EQ(outJson["messageModel"].asInt(), MessageModel::BROADCASTING);
  EXPECT_EQ(outJson["consumeFromWhere"].asInt(), ConsumeFromWhere::CONSUME_FROM_TIMESTAMP);

  Json::Value subsValue = outJson["subscriptionDataSet"];
  EXPECT_EQ(subsValue[0]["topic"], "testTopic");
  EXPECT_EQ(subsValue[0]["subString"], "sub");
}

TEST(HeartbeatDataTest, HeartbeatData) {
  HeartbeatData heartbeatData;
  heartbeatData.set_client_id("testClientId");

  EXPECT_TRUE(heartbeatData.producer_data_set().empty());
  heartbeatData.producer_data_set().emplace_back("testGroup");
  EXPECT_FALSE(heartbeatData.producer_data_set().empty());

  EXPECT_TRUE(heartbeatData.consumer_data_set().empty());
  heartbeatData.consumer_data_set().emplace_back("testGroup", ConsumeType::CONSUME_ACTIVELY, MessageModel::BROADCASTING,
                                                 ConsumeFromWhere::CONSUME_FROM_TIMESTAMP,
                                                 std::vector<SubscriptionData>{SubscriptionData("testTopic", "sub")});
  EXPECT_FALSE(heartbeatData.consumer_data_set().empty());

  std::string outData = heartbeatData.encode();

  Json::Value root;
  Json::Reader reader;
  reader.parse(outData, root);

  EXPECT_EQ(root["clientID"], "testClientId");
}

int main(int argc, char* argv[]) {
  InitGoogleMock(&argc, argv);
  testing::GTEST_FLAG(throw_on_failure) = true;
  testing::GTEST_FLAG(filter) = "HeartbeatDataTest.*";
  return RUN_ALL_TESTS();
}
