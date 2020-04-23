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

#include "ConsumeStats.h"
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
using rocketmq::ConsumeStats;
using rocketmq::MessageQueue;
using rocketmq::ProcessQueueInfo;
using rocketmq::SubscriptionData;

TEST(ConsumerRunningInfo, init) {
  ConsumerRunningInfo info;

  // jstack
  info.setJstack("jstack");
  EXPECT_EQ(info.getJstack(), "jstack");

  // property
  EXPECT_TRUE(info.getProperties().empty());
  info.setProperty("testKey", "testValue");
  map<string, string> properties = info.getProperties();
  EXPECT_EQ(properties["testKey"], "testValue");
  info.setProperties(map<string, string>());
  EXPECT_TRUE(info.getProperties().empty());
  info.setProperty("testKey", "testValue");
  map<string, string> properties2 = info.getProperties();
  EXPECT_EQ(properties2["testKey"], "testValue");

  // subscription
  EXPECT_TRUE(info.getSubscriptionSet().empty());
  vector<SubscriptionData> subscriptionSet;
  SubscriptionData sData1("testTopic", "testSub");
  sData1.putTagsSet("testTag");
  sData1.putCodeSet("11234");
  subscriptionSet.push_back(sData1);
  SubscriptionData sData2("testTopic2", "testSub2");
  sData2.putTagsSet("testTag2");
  sData2.putCodeSet("21234");
  subscriptionSet.push_back(sData2);
  info.setSubscriptionSet(subscriptionSet);
  EXPECT_EQ(info.getSubscriptionSet().size(), 2);

  // mqtable
  EXPECT_TRUE(info.getMqTable().empty());

  MessageQueue messageQueue1("testTopic", "testBrokerA", 3);
  ProcessQueueInfo processQueueInfo1;
  processQueueInfo1.commitOffset = 1024;
  info.setMqTable(messageQueue1, processQueueInfo1);
  MessageQueue messageQueue2("testTopic", "testBrokerB", 4);
  ProcessQueueInfo processQueueInfo2;
  processQueueInfo2.cachedMsgCount = 1023;
  info.setMqTable(messageQueue2, processQueueInfo2);
  map<MessageQueue, ProcessQueueInfo> mqTable = info.getMqTable();
  EXPECT_EQ(mqTable.size(), 2);
  EXPECT_EQ(mqTable[messageQueue1].commitOffset, 1024);
  EXPECT_EQ(mqTable[messageQueue2].cachedMsgCount, 1023);
  // consumeStats
  EXPECT_TRUE(info.getStatusTable().empty());

  ConsumeStats consumeStats;
  consumeStats.pullTPS = 22.5;
  ConsumeStats consumeStats2;
  consumeStats2.consumeOKTPS = 3.168;
  info.setStatusTable("TopicA", consumeStats);
  info.setStatusTable("TopicB", consumeStats2);
  map<string, ConsumeStats> statsTable = info.getStatusTable();
  EXPECT_EQ(statsTable.size(), 2);
  EXPECT_EQ(statsTable["TopicA"].pullTPS, 22.5);
  EXPECT_EQ(statsTable["TopicB"].consumeOKTPS, 3.168);

  // encode start
  info.setProperty(ConsumerRunningInfo::PROP_NAMESERVER_ADDR, "127.0.0.1:9876");
  info.setProperty(ConsumerRunningInfo::PROP_THREADPOOL_CORE_SIZE, "core_size");
  info.setProperty(ConsumerRunningInfo::PROP_CONSUME_ORDERLY, "consume_orderly");
  info.setProperty(ConsumerRunningInfo::PROP_CONSUME_TYPE, "consume_type");
  info.setProperty(ConsumerRunningInfo::PROP_CLIENT_VERSION, "client_version");
  info.setProperty(ConsumerRunningInfo::PROP_CONSUMER_START_TIMESTAMP, "127");
  info.setProperty(ConsumerRunningInfo::PROP_CLIENT_SDK_VERSION, "sdk_version");

  // encode
  string outStr = info.encode();
  EXPECT_GT(outStr.length(), 1);
  EXPECT_NE(outStr.find("testTopic"), string::npos);
}

int main(int argc, char* argv[]) {
  InitGoogleMock(&argc, argv);
  testing::GTEST_FLAG(throw_on_failure) = true;
  testing::GTEST_FLAG(filter) = "ConsumerRunningInfo.*";
  int iTests = RUN_ALL_TESTS();
  return iTests;
}
