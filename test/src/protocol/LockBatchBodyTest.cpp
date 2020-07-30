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

#include "ByteArray.h"
#include "MQMessageQueue.h"
#include "protocol/body/LockBatchRequestBody.hpp"
#include "protocol/body/LockBatchResponseBody.hpp"
#include "protocol/body/UnlockBatchRequestBody.hpp"

using testing::InitGoogleMock;
using testing::InitGoogleTest;
using testing::Return;

using rocketmq::ByteArray;
using rocketmq::LockBatchRequestBody;
using rocketmq::LockBatchResponseBody;
using rocketmq::MQMessageQueue;
using rocketmq::UnlockBatchRequestBody;

TEST(LockBatchBodyTest, LockBatchRequestBody) {
  LockBatchRequestBody lockBatchRequestBody;

  lockBatchRequestBody.set_client_id("testClientId");
  EXPECT_EQ(lockBatchRequestBody.client_id(), "testClientId");

  lockBatchRequestBody.set_consumer_group("testGroup");
  EXPECT_EQ(lockBatchRequestBody.consumer_group(), "testGroup");

  std::vector<MQMessageQueue> messageQueueList;
  messageQueueList.push_back(MQMessageQueue("testTopic", "testBroker", 1));
  messageQueueList.push_back(MQMessageQueue("testTopic", "testBroker", 2));

  lockBatchRequestBody.set_mq_set(messageQueueList);
  EXPECT_EQ(lockBatchRequestBody.mq_set(), messageQueueList);

  std::string outData = lockBatchRequestBody.encode();

  Json::Value root;
  Json::Reader reader;
  reader.parse(outData, root);
  EXPECT_EQ(root["clientId"], "testClientId");
  EXPECT_EQ(root["consumerGroup"], "testGroup");
  EXPECT_EQ(root["mqSet"][1]["topic"], "testTopic");
  EXPECT_EQ(root["mqSet"][1]["brokerName"], "testBroker");
  EXPECT_EQ(root["mqSet"][1]["queueId"], 2);
}

TEST(LockBatchBodyTest, UnlockBatchRequestBody) {}

TEST(LockBatchBodyTest, LockBatchResponseBody) {
  Json::Value root;
  Json::Value mqs;

  Json::Value mq;
  mq["topic"] = "testTopic";
  mq["brokerName"] = "testBroker";
  mq["queueId"] = 1;
  mqs[0] = mq;
  root["lockOKMQSet"] = mqs;

  Json::FastWriter fastwrite;
  std::string data = fastwrite.write(root);

  const ByteArray bodyData((char*)data.data(), data.size());
  std::unique_ptr<LockBatchResponseBody> lockBatchResponseBody(LockBatchResponseBody::Decode(bodyData));

  MQMessageQueue messageQueue("testTopic", "testBroker", 1);
  EXPECT_EQ(messageQueue, lockBatchResponseBody->lock_ok_mq_set()[0]);
}

int main(int argc, char* argv[]) {
  InitGoogleMock(&argc, argv);
  testing::GTEST_FLAG(throw_on_failure) = true;
  testing::GTEST_FLAG(filter) = "LockBatchBodyTest.*";
  return RUN_ALL_TESTS();
}
