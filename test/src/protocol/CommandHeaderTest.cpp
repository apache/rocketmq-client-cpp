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
#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "json/value.h"
#include "json/writer.h"

#include "CommandHeader.h"
#include "MQClientException.h"
#include "MessageSysFlag.h"
#include "UtilAll.h"
#include "dataBlock.h"
#include "json/json.h"

using std::shared_ptr;

using ::testing::InitGoogleMock;
using ::testing::InitGoogleTest;
using testing::Return;

using Json::FastWriter;
using Json::Value;

using rocketmq::CommandHeader;
using rocketmq::ConsumerSendMsgBackRequestHeader;
using rocketmq::CreateTopicRequestHeader;
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
using rocketmq::SendMessageResponseHeader;
using rocketmq::UnregisterClientRequestHeader;
using rocketmq::UpdateConsumerOffsetRequestHeader;
using rocketmq::ViewMessageRequestHeader;

TEST(commandHeader, ConsumerSendMsgBackRequestHeader) {}

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
  EXPECT_EQ(requestMap["jstackEnable"], "1");

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
