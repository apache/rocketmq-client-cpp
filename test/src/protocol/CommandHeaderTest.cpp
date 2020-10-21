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

#include <memory>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "json/value.h"
#include "json/writer.h"

#include "CommandHeader.h"
#include "dataBlock.h"

using std::shared_ptr;

using ::testing::InitGoogleMock;
using ::testing::InitGoogleTest;
using testing::Return;

using Json::FastWriter;
using Json::Value;

using rocketmq::CheckTransactionStateRequestHeader;
using rocketmq::CommandHeader;
using rocketmq::ConsumerSendMsgBackRequestHeader;
using rocketmq::CreateTopicRequestHeader;
using rocketmq::EndTransactionRequestHeader;
using rocketmq::GetConsumerListByGroupRequestHeader;
using rocketmq::GetConsumerListByGroupResponseBody;
using rocketmq::GetConsumerListByGroupResponseHeader;
using rocketmq::GetConsumerRunningInfoRequestHeader;
using rocketmq::GetEarliestMsgStoretimeRequestHeader;
using rocketmq::GetEarliestMsgStoretimeResponseHeader;
using rocketmq::GetMaxOffsetRequestHeader;
using rocketmq::GetMaxOffsetResponseHeader;
using rocketmq::GetMinOffsetRequestHeader;
using rocketmq::GetMinOffsetResponseHeader;
using rocketmq::GetRouteInfoRequestHeader;
using rocketmq::MemoryBlock;
using rocketmq::NotifyConsumerIdsChangedRequestHeader;
using rocketmq::PullMessageRequestHeader;
using rocketmq::PullMessageResponseHeader;
using rocketmq::QueryConsumerOffsetRequestHeader;
using rocketmq::QueryConsumerOffsetResponseHeader;
using rocketmq::ResetOffsetRequestHeader;
using rocketmq::SearchOffsetRequestHeader;
using rocketmq::SearchOffsetResponseHeader;
using rocketmq::SendMessageRequestHeader;
using rocketmq::SendMessageRequestHeaderV2;
using rocketmq::SendMessageResponseHeader;
using rocketmq::UnregisterClientRequestHeader;
using rocketmq::UpdateConsumerOffsetRequestHeader;
using rocketmq::ViewMessageRequestHeader;

TEST(commandHeader, ConsumerSendMsgBackRequestHeader) {
  string group = "testGroup";
  int delayLevel = 2;
  int64 offset = 3027;
  bool unitMode = true;
  string originMsgId = "testOriginMsgId";
  string originTopic = "testTopic";
  int maxReconsumeTimes = 12;
  ConsumerSendMsgBackRequestHeader header;
  header.group = group;
  header.delayLevel = delayLevel;
  header.offset = offset;
  header.unitMode = unitMode;
  header.originMsgId = originMsgId;
  header.originTopic = originTopic;
  header.maxReconsumeTimes = maxReconsumeTimes;
  map<string, string> requestMap;
  header.SetDeclaredFieldOfCommandHeader(requestMap);
  EXPECT_EQ(requestMap["group"], group);
  EXPECT_EQ(requestMap["delayLevel"], "2");
  EXPECT_EQ(requestMap["offset"], "3027");
  EXPECT_EQ(requestMap["unitMode"], "true");
  EXPECT_EQ(requestMap["originMsgId"], originMsgId);
  EXPECT_EQ(requestMap["originTopic"], originTopic);
  EXPECT_EQ(requestMap["maxReconsumeTimes"], "12");

  Value outData;
  header.Encode(outData);
  EXPECT_EQ(outData["group"], group);
  EXPECT_EQ(outData["delayLevel"], 2);
  EXPECT_EQ(outData["offset"], "3027");
  EXPECT_EQ(outData["unitMode"], "true");
  EXPECT_EQ(outData["originMsgId"], originMsgId);
  EXPECT_EQ(outData["originTopic"], originTopic);
  EXPECT_EQ(outData["maxReconsumeTimes"], 12);
}

TEST(commandHeader, GetRouteInfoRequestHeader) {
  GetRouteInfoRequestHeader header("testTopic");
  map<string, string> requestMap;
  header.SetDeclaredFieldOfCommandHeader(requestMap);
  EXPECT_EQ(requestMap["topic"], "testTopic");

  Value outData;
  header.Encode(outData);
  EXPECT_EQ(outData["topic"], "testTopic");
}

TEST(commandHeader, UnregisterClientRequestHeader) {
  UnregisterClientRequestHeader header("testGroup", "testProducer", "testConsumer");
  map<string, string> requestMap;
  header.SetDeclaredFieldOfCommandHeader(requestMap);
  EXPECT_EQ(requestMap["clientID"], "testGroup");
  EXPECT_EQ(requestMap["producerGroup"], "testProducer");
  EXPECT_EQ(requestMap["consumerGroup"], "testConsumer");

  Value outData;
  header.Encode(outData);
  EXPECT_EQ(outData["clientID"], "testGroup");
  EXPECT_EQ(outData["producerGroup"], "testProducer");
  EXPECT_EQ(outData["consumerGroup"], "testConsumer");
}

TEST(commandHeader, CreateTopicRequestHeader) {
  string topic = "testTopic";
  string defaultTopic = "defaultTopic";
  int readQueueNums = 4;
  int writeQueueNums = 6;
  int perm = 8;
  string topicFilterType = "filterType";
  CreateTopicRequestHeader header;
  header.topic = topic;
  header.defaultTopic = defaultTopic;
  header.readQueueNums = readQueueNums;
  header.writeQueueNums = writeQueueNums;
  header.perm = perm;
  header.topicFilterType = topicFilterType;
  map<string, string> requestMap;
  header.SetDeclaredFieldOfCommandHeader(requestMap);
  EXPECT_EQ(requestMap["topic"], topic);
  EXPECT_EQ(requestMap["defaultTopic"], defaultTopic);
  EXPECT_EQ(requestMap["readQueueNums"], "4");
  EXPECT_EQ(requestMap["writeQueueNums"], "6");
  EXPECT_EQ(requestMap["perm"], "8");
  EXPECT_EQ(requestMap["topicFilterType"], topicFilterType);

  Value outData;
  header.Encode(outData);
  EXPECT_EQ(outData["topic"], topic);
  EXPECT_EQ(outData["defaultTopic"], defaultTopic);
  EXPECT_EQ(outData["readQueueNums"], readQueueNums);
  EXPECT_EQ(outData["writeQueueNums"], writeQueueNums);
  EXPECT_EQ(outData["perm"], perm);
  EXPECT_EQ(outData["topicFilterType"], topicFilterType);
}

TEST(commandHeader, CheckTransactionStateRequestHeader) {
  CheckTransactionStateRequestHeader header(2000, 1000, "ABC", "DEF", "GHI");
  map<string, string> requestMap;
  header.SetDeclaredFieldOfCommandHeader(requestMap);
  EXPECT_EQ(requestMap["msgId"], "ABC");
  EXPECT_EQ(requestMap["transactionId"], "DEF");
  EXPECT_EQ(requestMap["offsetMsgId"], "GHI");
  EXPECT_EQ(requestMap["commitLogOffset"], "1000");
  EXPECT_EQ(requestMap["tranStateTableOffset"], "2000");

  Value value;
  value["msgId"] = "ABC";
  value["transactionId"] = "DEF";
  value["offsetMsgId"] = "GHI";
  value["commitLogOffset"] = "1000";
  value["tranStateTableOffset"] = "2000";
  shared_ptr<CheckTransactionStateRequestHeader> headerDecode(
      static_cast<CheckTransactionStateRequestHeader*>(CheckTransactionStateRequestHeader::Decode(value)));
  EXPECT_EQ(headerDecode->m_msgId, "ABC");
  EXPECT_EQ(headerDecode->m_commitLogOffset, 1000);
  EXPECT_EQ(headerDecode->m_tranStateTableOffset, 2000);
  EXPECT_EQ(headerDecode->toString(), header.toString());
}

TEST(commandHeader, EndTransactionRequestHeader) {
  EndTransactionRequestHeader header("testProducer", 1000, 2000, 3000, true, "ABC", "DEF");
  map<string, string> requestMap;
  header.SetDeclaredFieldOfCommandHeader(requestMap);
  EXPECT_EQ(requestMap["msgId"], "ABC");
  EXPECT_EQ(requestMap["transactionId"], "DEF");
  EXPECT_EQ(requestMap["producerGroup"], "testProducer");
  EXPECT_EQ(requestMap["tranStateTableOffset"], "1000");
  EXPECT_EQ(requestMap["commitLogOffset"], "2000");
  EXPECT_EQ(requestMap["commitOrRollback"], "3000");
  EXPECT_EQ(requestMap["fromTransactionCheck"], "true");

  Value outData;
  header.Encode(outData);
  EXPECT_EQ(outData["msgId"], "ABC");
  EXPECT_EQ(outData["transactionId"], "DEF");
  EXPECT_EQ(outData["producerGroup"], "testProducer");
  EXPECT_EQ(outData["tranStateTableOffset"], "1000");
  EXPECT_EQ(outData["commitLogOffset"], "2000");
  EXPECT_EQ(outData["commitOrRollback"], "3000");
  EXPECT_EQ(outData["fromTransactionCheck"], "true");

  EXPECT_NO_THROW(header.toString());
}

TEST(commandHeader, SendMessageRequestHeader) {
  string producerGroup = "testProducer";
  string topic = "testTopic";
  string defaultTopic = "defaultTopic";
  int defaultTopicQueueNums = 1;
  int queueId = 2;
  int sysFlag = 3;
  int64 bornTimestamp = 4;
  int flag = 5;
  string properties = "testProperty";
  int reconsumeTimes = 6;
  bool unitMode = true;
  bool batch = false;

  SendMessageRequestHeader header;
  header.producerGroup = producerGroup;
  header.topic = topic;
  header.defaultTopic = defaultTopic;
  header.defaultTopicQueueNums = defaultTopicQueueNums;
  header.queueId = queueId;
  header.sysFlag = sysFlag;
  header.bornTimestamp = bornTimestamp;
  header.flag = flag;
  header.properties = properties;
  header.reconsumeTimes = reconsumeTimes;
  header.unitMode = unitMode;
  header.batch = batch;
  map<string, string> requestMap;
  header.SetDeclaredFieldOfCommandHeader(requestMap);
  EXPECT_EQ(requestMap["topic"], topic);
  EXPECT_EQ(requestMap["producerGroup"], producerGroup);
  EXPECT_EQ(requestMap["defaultTopic"], defaultTopic);
  EXPECT_EQ(requestMap["defaultTopicQueueNums"], "1");
  EXPECT_EQ(requestMap["queueId"], "2");
  EXPECT_EQ(requestMap["sysFlag"], "3");
  EXPECT_EQ(requestMap["bornTimestamp"], "4");
  EXPECT_EQ(requestMap["flag"], "5");
  EXPECT_EQ(requestMap["properties"], properties);
  EXPECT_EQ(requestMap["reconsumeTimes"], "6");
  EXPECT_EQ(requestMap["unitMode"], "true");
  EXPECT_EQ(requestMap["batch"], "false");

  Value outData;
  header.Encode(outData);
  EXPECT_EQ(outData["topic"], topic);
  EXPECT_EQ(outData["producerGroup"], producerGroup);
  EXPECT_EQ(outData["defaultTopic"], defaultTopic);
  EXPECT_EQ(outData["defaultTopicQueueNums"], defaultTopicQueueNums);
  EXPECT_EQ(outData["queueId"], queueId);
  EXPECT_EQ(outData["sysFlag"], sysFlag);
  EXPECT_EQ(outData["bornTimestamp"], "4");
  EXPECT_EQ(outData["flag"], flag);
  EXPECT_EQ(outData["properties"], properties);
  EXPECT_EQ(outData["reconsumeTimes"], "6");
  EXPECT_EQ(outData["unitMode"], "true");
  EXPECT_EQ(outData["batch"], "false");
}

TEST(commandHeader, SendMessageRequestHeaderV2) {
  string producerGroup = "testProducer";
  string topic = "testTopic";
  string defaultTopic = "defaultTopic";
  int defaultTopicQueueNums = 1;
  int queueId = 2;
  int sysFlag = 3;
  int64 bornTimestamp = 4;
  int flag = 5;
  string properties = "testProperty";
  int reconsumeTimes = 6;
  bool unitMode = true;
  bool batch = false;

  SendMessageRequestHeaderV2 header;
  header.a = producerGroup;
  header.b = topic;
  header.c = defaultTopic;
  header.d = defaultTopicQueueNums;
  header.e = queueId;
  header.f = sysFlag;
  header.g = bornTimestamp;
  header.h = flag;
  header.i = properties;
  header.j = reconsumeTimes;
  header.k = unitMode;
  header.m = batch;
  map<string, string> requestMap;
  header.SetDeclaredFieldOfCommandHeader(requestMap);
  EXPECT_EQ(requestMap["a"], producerGroup);
  EXPECT_EQ(requestMap["b"], topic);
  EXPECT_EQ(requestMap["c"], defaultTopic);
  EXPECT_EQ(requestMap["d"], "1");
  EXPECT_EQ(requestMap["e"], "2");
  EXPECT_EQ(requestMap["f"], "3");
  EXPECT_EQ(requestMap["g"], "4");
  EXPECT_EQ(requestMap["h"], "5");
  EXPECT_EQ(requestMap["i"], properties);
  EXPECT_EQ(requestMap["j"], "6");
  EXPECT_EQ(requestMap["k"], "true");
  EXPECT_EQ(requestMap["m"], "false");

  Value outData;
  header.Encode(outData);
  EXPECT_EQ(outData["a"], producerGroup);
  EXPECT_EQ(outData["b"], topic);
  EXPECT_EQ(outData["c"], defaultTopic);
  EXPECT_EQ(outData["d"], defaultTopicQueueNums);
  EXPECT_EQ(outData["e"], queueId);
  EXPECT_EQ(outData["f"], sysFlag);
  EXPECT_EQ(outData["g"], "4");
  EXPECT_EQ(outData["h"], flag);
  EXPECT_EQ(outData["i"], properties);
  EXPECT_EQ(outData["j"], "6");
  EXPECT_EQ(outData["k"], "true");
  EXPECT_EQ(outData["m"], "false");

  SendMessageRequestHeader v1;
  header.CreateSendMessageRequestHeaderV1(v1);
  EXPECT_EQ(v1.producerGroup, producerGroup);
  EXPECT_EQ(v1.queueId, queueId);
  EXPECT_EQ(v1.batch, batch);

  SendMessageRequestHeaderV2 v2(v1);
  EXPECT_EQ(header.a, v2.a);
  EXPECT_EQ(header.e, v2.e);
  EXPECT_EQ(header.m, v2.m);
  EXPECT_EQ(header.g, v2.g);
}

TEST(commandHeader, SendMessageResponseHeader) {
  SendMessageResponseHeader header;
  header.msgId = "ABCDEFG";
  header.queueId = 1;
  header.queueOffset = 2;
  header.transactionId = "ID";
  header.regionId = "public";
  map<string, string> requestMap;
  header.SetDeclaredFieldOfCommandHeader(requestMap);
  EXPECT_EQ(requestMap["msgId"], "ABCDEFG");
  EXPECT_EQ(requestMap["queueId"], "1");
  EXPECT_EQ(requestMap["queueOffset"], "2");
  EXPECT_EQ(requestMap["transactionId"], "ID");
  EXPECT_EQ(requestMap["MSG_REGION"], "public");

  Value value;
  value["msgId"] = "EFGHIJK";
  value["queueId"] = "3";
  value["queueOffset"] = "4";
  value["transactionId"] = "transactionId";
  value["MSG_REGION"] = "MSG_REGION";
  shared_ptr<SendMessageResponseHeader> headerDecode(
      static_cast<SendMessageResponseHeader*>(SendMessageResponseHeader::Decode(value)));
  EXPECT_EQ(headerDecode->msgId, "EFGHIJK");
  EXPECT_EQ(headerDecode->queueId, 3);
  EXPECT_EQ(headerDecode->queueOffset, 4);
  EXPECT_EQ(headerDecode->transactionId, "transactionId");
  EXPECT_EQ(headerDecode->regionId, "MSG_REGION");
}

TEST(commandHeader, PullMessageRequestHeader) {
  PullMessageRequestHeader header;
  header.consumerGroup = "testConsumer";
  header.topic = "testTopic";
  header.queueId = 1;
  header.maxMsgNums = 2;
  header.sysFlag = 3;
  header.subscription = "testSub";
  header.queueOffset = 4;
  header.commitOffset = 5;
  header.suspendTimeoutMillis = 6;
  header.subVersion = 7;
  map<string, string> requestMap;
  header.SetDeclaredFieldOfCommandHeader(requestMap);
  EXPECT_EQ(requestMap["consumerGroup"], "testConsumer");
  EXPECT_EQ(requestMap["topic"], "testTopic");
  EXPECT_EQ(requestMap["queueId"], "1");
  EXPECT_EQ(requestMap["maxMsgNums"], "2");
  EXPECT_EQ(requestMap["sysFlag"], "3");
  EXPECT_EQ(requestMap["subscription"], "testSub");
  EXPECT_EQ(requestMap["queueOffset"], "4");
  EXPECT_EQ(requestMap["commitOffset"], "5");
  EXPECT_EQ(requestMap["suspendTimeoutMillis"], "6");
  EXPECT_EQ(requestMap["subVersion"], "7");

  Value outData;
  header.Encode(outData);
  EXPECT_EQ(outData["consumerGroup"], "testConsumer");
  EXPECT_EQ(outData["topic"], "testTopic");
  EXPECT_EQ(outData["queueId"], 1);
  EXPECT_EQ(outData["maxMsgNums"], 2);
  EXPECT_EQ(outData["sysFlag"], 3);
  EXPECT_EQ(outData["subscription"], "testSub");
  EXPECT_EQ(outData["queueOffset"], "4");
  EXPECT_EQ(outData["commitOffset"], "5");
  EXPECT_EQ(outData["suspendTimeoutMillis"], "6");
  EXPECT_EQ(outData["subVersion"], "7");
}

TEST(commandHeader, PullMessageResponseHeader) {
  PullMessageResponseHeader header;
  header.suggestWhichBrokerId = 100;
  header.nextBeginOffset = 200;
  header.minOffset = 3000;
  header.maxOffset = 5000;
  map<string, string> requestMap;
  header.SetDeclaredFieldOfCommandHeader(requestMap);
  EXPECT_EQ(requestMap["suggestWhichBrokerId"], "100");
  EXPECT_EQ(requestMap["nextBeginOffset"], "200");
  EXPECT_EQ(requestMap["minOffset"], "3000");
  EXPECT_EQ(requestMap["maxOffset"], "5000");

  Value value;
  value["suggestWhichBrokerId"] = "5";
  value["nextBeginOffset"] = "102400";
  value["minOffset"] = "1";
  value["maxOffset"] = "123456789";
  shared_ptr<PullMessageResponseHeader> headerDecode(
      static_cast<PullMessageResponseHeader*>(PullMessageResponseHeader::Decode(value)));
  EXPECT_EQ(headerDecode->suggestWhichBrokerId, 5);
  EXPECT_EQ(headerDecode->nextBeginOffset, 102400);
  EXPECT_EQ(headerDecode->minOffset, 1);
  EXPECT_EQ(headerDecode->maxOffset, 123456789);
}

TEST(commandHeader, GetConsumerListByGroupResponseBody) {
  Value value;
  value[0] = "body";
  value[1] = 1;

  Value root;
  root["consumerIdList"] = value;

  FastWriter writer;
  string data = writer.write(root);

  MemoryBlock* mem = new MemoryBlock(data.c_str(), data.size());
  vector<string> cids;
  GetConsumerListByGroupResponseBody::Decode(mem, cids);

  EXPECT_EQ(cids.size(), 1);

  delete mem;
}

TEST(commandHeader, ResetOffsetRequestHeader) {
  ResetOffsetRequestHeader header;

  header.setTopic("testTopic");
  EXPECT_EQ(header.getTopic(), "testTopic");

  header.setGroup("testGroup");
  EXPECT_EQ(header.getGroup(), "testGroup");

  header.setTimeStamp(123);
  EXPECT_EQ(header.getTimeStamp(), 123);

  header.setForceFlag(true);
  EXPECT_TRUE(header.getForceFlag());

  Value value;
  value["isForce"] = "false";
  shared_ptr<ResetOffsetRequestHeader> headersh(
      static_cast<ResetOffsetRequestHeader*>(ResetOffsetRequestHeader::Decode(value)));
  EXPECT_EQ(headersh->getTopic(), "");
  EXPECT_EQ(headersh->getGroup(), "");
  // EXPECT_EQ(headersh->getTimeStamp(), 0);
  EXPECT_FALSE(headersh->getForceFlag());
  value["topic"] = "testTopic";
  headersh.reset(static_cast<ResetOffsetRequestHeader*>(ResetOffsetRequestHeader::Decode(value)));
  EXPECT_EQ(headersh->getTopic(), "testTopic");
  EXPECT_EQ(headersh->getGroup(), "");
  // EXPECT_EQ(headersh->getTimeStamp(), 0);
  EXPECT_FALSE(headersh->getForceFlag());

  value["topic"] = "testTopic";
  value["group"] = "testGroup";
  headersh.reset(static_cast<ResetOffsetRequestHeader*>(ResetOffsetRequestHeader::Decode(value)));
  EXPECT_EQ(headersh->getTopic(), "testTopic");
  EXPECT_EQ(headersh->getGroup(), "testGroup");
  // EXPECT_EQ(headersh->getTimeStamp(), 0);
  EXPECT_FALSE(headersh->getForceFlag());

  value["topic"] = "testTopic";
  value["group"] = "testGroup";
  value["timestamp"] = "123";
  headersh.reset(static_cast<ResetOffsetRequestHeader*>(ResetOffsetRequestHeader::Decode(value)));
  EXPECT_EQ(headersh->getTopic(), "testTopic");
  EXPECT_EQ(headersh->getGroup(), "testGroup");
  EXPECT_EQ(headersh->getTimeStamp(), 123);
  EXPECT_FALSE(headersh->getForceFlag());

  value["topic"] = "testTopic";
  value["group"] = "testGroup";
  value["timestamp"] = "123";
  value["isForce"] = "1";
  headersh.reset(static_cast<ResetOffsetRequestHeader*>(ResetOffsetRequestHeader::Decode(value)));
  EXPECT_EQ(headersh->getTopic(), "testTopic");
  EXPECT_EQ(headersh->getGroup(), "testGroup");
  EXPECT_EQ(headersh->getTimeStamp(), 123);
  EXPECT_TRUE(headersh->getForceFlag());
}

TEST(commandHeader, GetConsumerRunningInfoRequestHeader) {
  GetConsumerRunningInfoRequestHeader header;
  header.setClientId("testClientId");
  header.setConsumerGroup("testConsumer");
  header.setJstackEnable(true);

  map<string, string> requestMap;
  header.SetDeclaredFieldOfCommandHeader(requestMap);
  EXPECT_EQ(requestMap["clientId"], "testClientId");
  EXPECT_EQ(requestMap["consumerGroup"], "testConsumer");
  EXPECT_EQ(requestMap["jstackEnable"], "true");

  Value outData;
  header.Encode(outData);
  EXPECT_EQ(outData["clientId"], "testClientId");
  EXPECT_EQ(outData["consumerGroup"], "testConsumer");
  EXPECT_TRUE(outData["jstackEnable"].asBool());

  shared_ptr<GetConsumerRunningInfoRequestHeader> decodeHeader(
      static_cast<GetConsumerRunningInfoRequestHeader*>(GetConsumerRunningInfoRequestHeader::Decode(outData)));
  EXPECT_EQ(decodeHeader->getClientId(), "testClientId");
  EXPECT_EQ(decodeHeader->getConsumerGroup(), "testConsumer");
  EXPECT_TRUE(decodeHeader->isJstackEnable());
}

TEST(commandHeader, NotifyConsumerIdsChangedRequestHeader) {
  Json::Value ext;
  shared_ptr<NotifyConsumerIdsChangedRequestHeader> header(
      static_cast<NotifyConsumerIdsChangedRequestHeader*>(NotifyConsumerIdsChangedRequestHeader::Decode(ext)));
  EXPECT_EQ(header->getGroup(), "");

  ext["consumerGroup"] = "testGroup";
  header.reset(static_cast<NotifyConsumerIdsChangedRequestHeader*>(NotifyConsumerIdsChangedRequestHeader::Decode(ext)));
  EXPECT_EQ(header->getGroup(), "testGroup");
}

int main(int argc, char* argv[]) {
  InitGoogleMock(&argc, argv);
  testing::GTEST_FLAG(throw_on_failure) = true;
  testing::GTEST_FLAG(filter) = "commandHeader.*";
  int itestts = RUN_ALL_TESTS();
  return itestts;
}
