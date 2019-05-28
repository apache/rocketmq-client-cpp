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
#include <iostream>
#include "map"
#include "string"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "json/reader.h"
#include "json/value.h"

#include "ConsumerRunningInfo.h"
#include "MessageQueue.h"
#include "ProcessQueueInfo.h"
#include "SubscriptionData.h"

using std::map;
using std::string;

using ::testing::InitGoogleMock;
using ::testing::InitGoogleTest;
using testing::Return;

using Json::Reader;
using Json::Value;

using rocketmq::ConsumerRunningInfo;
using rocketmq::MessageQueue;
using rocketmq::ProcessQueueInfo;
using rocketmq::SubscriptionData;

TEST(consumerRunningInfo, init) {
  ConsumerRunningInfo consumerRunningInfo;
  consumerRunningInfo.setJstack("jstack");
  EXPECT_EQ(consumerRunningInfo.getJstack(), "jstack");

  EXPECT_TRUE(consumerRunningInfo.getProperties().empty());

  consumerRunningInfo.setProperty("testKey", "testValue");
  map<string, string> properties = consumerRunningInfo.getProperties();
  EXPECT_EQ(properties["testKey"], "testValue");

  consumerRunningInfo.setProperties(map<string, string>());
  EXPECT_TRUE(consumerRunningInfo.getProperties().empty());

  EXPECT_TRUE(consumerRunningInfo.getSubscriptionSet().empty());

  vector<SubscriptionData> subscriptionSet;
  subscriptionSet.push_back(SubscriptionData());

  consumerRunningInfo.setSubscriptionSet(subscriptionSet);
  EXPECT_EQ(consumerRunningInfo.getSubscriptionSet().size(), 1);

  EXPECT_TRUE(consumerRunningInfo.getMqTable().empty());

  MessageQueue messageQueue("testTopic", "testBroker", 3);
  ProcessQueueInfo processQueueInfo;
  processQueueInfo.commitOffset = 1024;
  consumerRunningInfo.setMqTable(messageQueue, processQueueInfo);
  map<MessageQueue, ProcessQueueInfo> mqTable = consumerRunningInfo.getMqTable();
  EXPECT_EQ(mqTable[messageQueue].commitOffset, processQueueInfo.commitOffset);

  // encode start
  consumerRunningInfo.setProperty(ConsumerRunningInfo::PROP_NAMESERVER_ADDR, "127.0.0.1:9876");
  consumerRunningInfo.setProperty(ConsumerRunningInfo::PROP_THREADPOOL_CORE_SIZE, "core_size");
  consumerRunningInfo.setProperty(ConsumerRunningInfo::PROP_CONSUME_ORDERLY, "consume_orderly");
  consumerRunningInfo.setProperty(ConsumerRunningInfo::PROP_CONSUME_TYPE, "consume_type");
  consumerRunningInfo.setProperty(ConsumerRunningInfo::PROP_CLIENT_VERSION, "client_version");
  consumerRunningInfo.setProperty(ConsumerRunningInfo::PROP_CONSUMER_START_TIMESTAMP, "127");
  // TODO
  /* string outstr = consumerRunningInfo.encode();
   std::cout<< outstr;
   Value root;
   Reader reader;
   reader.parse(outstr.c_str(), root);

   EXPECT_EQ(root["jstack"].asString() , "jstack");

   Json::Value outData = root["properties"];
   EXPECT_EQ(outData[ConsumerRunningInfo::PROP_NAMESERVER_ADDR].asString(),"127.0.0.1:9876");
   EXPECT_EQ(
           outData[ConsumerRunningInfo::PROP_THREADPOOL_CORE_SIZE].asString(),
           "core_size");
   EXPECT_EQ(outData[ConsumerRunningInfo::PROP_CONSUME_ORDERLY].asString(),
             "consume_orderly");
   EXPECT_EQ(outData[ConsumerRunningInfo::PROP_CONSUME_TYPE].asString(),
             "consume_type");
   EXPECT_EQ(outData[ConsumerRunningInfo::PROP_CLIENT_VERSION].asString(),
             "client_version");
   EXPECT_EQ(
           outData[ConsumerRunningInfo::PROP_CONSUMER_START_TIMESTAMP].asString(),
           "127");

   Json::Value subscriptionSetJson = root["subscriptionSet"];
   EXPECT_EQ(subscriptionSetJson[0], subscriptionSet[0].toJson());

   Json::Value mqTableJson = root["mqTable"];
   EXPECT_EQ(mqTableJson[messageQueue.toJson().toStyledString()].asString(),
             processQueueInfo.toJson().toStyledString());
*/
}

int main(int argc, char* argv[]) {
  InitGoogleMock(&argc, argv);
  testing::GTEST_FLAG(throw_on_failure) = true;
  testing::GTEST_FLAG(filter) = "consumerRunningInfo.*";
  int itestts = RUN_ALL_TESTS();
  return itestts;
}
