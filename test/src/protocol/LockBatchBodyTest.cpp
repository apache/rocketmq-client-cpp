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

#include "LockBatchBody.h"
#include "MQMessageQueue.h"
#include "dataBlock.h"

using std::vector;

using ::testing::InitGoogleMock;
using ::testing::InitGoogleTest;
using testing::Return;

using rocketmq::LockBatchRequestBody;
using rocketmq::LockBatchResponseBody;
using rocketmq::MemoryBlock;
using rocketmq::MQMessageQueue;
using rocketmq::UnlockBatchRequestBody;

TEST(lockBatchBody, LockBatchRequestBody) {
  LockBatchRequestBody lockBatchRequestBody;

  lockBatchRequestBody.setClientId("testClientId");
  EXPECT_EQ(lockBatchRequestBody.getClientId(), "testClientId");

  lockBatchRequestBody.setConsumerGroup("testGroup");
  EXPECT_EQ(lockBatchRequestBody.getConsumerGroup(), "testGroup");

  vector<MQMessageQueue> vectorMessageQueue;
  vectorMessageQueue.push_back(MQMessageQueue());
  vectorMessageQueue.push_back(MQMessageQueue());

  lockBatchRequestBody.setMqSet(vectorMessageQueue);
  EXPECT_EQ(lockBatchRequestBody.getMqSet(), vectorMessageQueue);

  Json::Value outJson = lockBatchRequestBody.toJson(MQMessageQueue("topicTest", "testBroker", 10));
  EXPECT_EQ(outJson["topic"], "topicTest");
  EXPECT_EQ(outJson["brokerName"], "testBroker");
  EXPECT_EQ(outJson["queueId"], 10);

  string outData;
  lockBatchRequestBody.Encode(outData);

  Json::Value root;
  Json::Reader reader;
  reader.parse(outData, root);
  EXPECT_EQ(root["clientId"], "testClientId");
  EXPECT_EQ(root["consumerGroup"], "testGroup");
}

TEST(lockBatchBody, UnlockBatchRequestBody) {
  UnlockBatchRequestBody uRB;
  uRB.setClientId("testClient");
  EXPECT_EQ(uRB.getClientId(), "testClient");
  uRB.setConsumerGroup("testGroup");
  EXPECT_EQ(uRB.getConsumerGroup(), "testGroup");

  // message queue
  EXPECT_TRUE(uRB.getMqSet().empty());
  vector<MQMessageQueue> mqs;
  MQMessageQueue mqA("testTopic", "testBrokerA", 1);
  mqs.push_back(mqA);
  MQMessageQueue mqB("testTopic", "testBrokerB", 2);
  mqs.push_back(mqB);
  uRB.setMqSet(mqs);
  EXPECT_EQ(uRB.getMqSet().size(), 2);
  string outData;
  uRB.Encode(outData);
  EXPECT_GT(outData.length(), 1);
  EXPECT_NE(outData.find("testTopic"), string::npos);
}

TEST(lockBatchBody, LockBatchResponseBody) {
  Json::Value root;
  Json::Value mqs;

  Json::Value mq;
  mq["topic"] = "testTopic";
  mq["brokerName"] = "testBroker";
  mq["queueId"] = 1;
  mqs[0] = mq;
  root["lockOKMQSet"] = mqs;

  Json::FastWriter fastwrite;
  string outData = fastwrite.write(root);

  MemoryBlock* mem = new MemoryBlock(outData.c_str(), outData.size());

  LockBatchResponseBody lockBatchResponseBody;

  vector<MQMessageQueue> messageQueues;
  LockBatchResponseBody::Decode(mem, messageQueues);

  MQMessageQueue messageQueue("testTopic", "testBroker", 1);
  EXPECT_EQ(messageQueue, messageQueues[0]);
}

int main(int argc, char* argv[]) {
  InitGoogleMock(&argc, argv);
  testing::GTEST_FLAG(throw_on_failure) = true;
  testing::GTEST_FLAG(filter) = "lockBatchBody.*";
  int itestts = RUN_ALL_TESTS();
  return itestts;
}
