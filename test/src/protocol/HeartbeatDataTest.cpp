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

#include "vector"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "ConsumeType.h"
#include "HeartbeatData.h"
#include "SubscriptionData.h"

using std::vector;

using ::testing::InitGoogleMock;
using ::testing::InitGoogleTest;
using testing::Return;

using rocketmq::ConsumeFromWhere;
using rocketmq::ConsumerData;
using rocketmq::ConsumeType;
using rocketmq::HeartbeatData;
using rocketmq::MessageModel;
using rocketmq::ProducerData;
using rocketmq::SubscriptionData;

TEST(heartbeatData, ProducerData) {
  ProducerData producerData;
  producerData.groupName = "testGroup";

  Json::Value outJson = producerData.toJson();
  EXPECT_EQ(outJson["groupName"], "testGroup");
}

TEST(heartbeatData, ConsumerData) {
  ConsumerData consumerData;
  consumerData.groupName = "testGroup";
  consumerData.consumeType = ConsumeType::CONSUME_ACTIVELY;
  consumerData.messageModel = MessageModel::BROADCASTING;
  consumerData.consumeFromWhere = ConsumeFromWhere::CONSUME_FROM_TIMESTAMP;

  vector<SubscriptionData> subs;
  subs.push_back(SubscriptionData("testTopic", "sub"));

  consumerData.subscriptionDataSet = subs;

  Json::Value outJson = consumerData.toJson();

  EXPECT_EQ(outJson["groupName"], "testGroup");

  EXPECT_EQ(outJson["consumeType"].asInt(), ConsumeType::CONSUME_ACTIVELY);
  EXPECT_EQ(outJson["messageModel"].asInt(), MessageModel::BROADCASTING);
  EXPECT_EQ(outJson["consumeFromWhere"].asInt(), ConsumeFromWhere::CONSUME_FROM_TIMESTAMP);

  Json::Value subsValue = outJson["subscriptionDataSet"];
  EXPECT_EQ(subsValue[0]["topic"], "testTopic");
  EXPECT_EQ(subsValue[0]["subString"], "sub");
}

TEST(heartbeatData, HeartbeatData) {
  HeartbeatData heartbeatData;
  heartbeatData.setClientID("testClientId");

  ProducerData producerData;
  producerData.groupName = "testGroup";

  EXPECT_TRUE(heartbeatData.isProducerDataSetEmpty());
  heartbeatData.insertDataToProducerDataSet(producerData);
  EXPECT_FALSE(heartbeatData.isProducerDataSetEmpty());

  ConsumerData consumerData;
  consumerData.groupName = "testGroup";
  consumerData.consumeType = ConsumeType::CONSUME_ACTIVELY;
  consumerData.messageModel = MessageModel::BROADCASTING;
  consumerData.consumeFromWhere = ConsumeFromWhere::CONSUME_FROM_TIMESTAMP;

  vector<SubscriptionData> subs;
  subs.push_back(SubscriptionData("testTopic", "sub"));

  consumerData.subscriptionDataSet = subs;
  EXPECT_TRUE(heartbeatData.isConsumerDataSetEmpty());
  heartbeatData.insertDataToConsumerDataSet(consumerData);
  EXPECT_FALSE(heartbeatData.isConsumerDataSetEmpty());

  string outData;
  heartbeatData.Encode(outData);

  Json::Value root;
  Json::Reader reader;
  reader.parse(outData, root);

  EXPECT_EQ(root["clientID"], "testClientId");
}

int main(int argc, char* argv[]) {
  InitGoogleMock(&argc, argv);
  testing::GTEST_FLAG(throw_on_failure) = true;
  testing::GTEST_FLAG(filter) = "heartbeatData.*";
  int itestts = RUN_ALL_TESTS();
  return itestts;
}
