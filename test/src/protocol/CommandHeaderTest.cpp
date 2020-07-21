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
#include <json/json.h>
#include <json/value.h>
#include <json/writer.h>

#include <map>
#include <memory>
#include <string>

#include "ByteArray.h"
#include "MQException.h"
#include "MessageSysFlag.h"
#include "UtilAll.h"
#include "protocol/header/CommandHeader.h"

using testing::InitGoogleMock;
using testing::InitGoogleTest;
using testing::Return;

using Json::FastWriter;
using Json::Value;

using rocketmq::ByteArray;
using rocketmq::CommandCustomHeader;
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

TEST(CommandHeaderTest, ConsumerSendMsgBackRequestHeader) {}

TEST(CommandHeaderTest, GetConsumerListByGroupResponseBody) {
  Value value;
  value[0] = "consumer1";
  value[1] = "consumer2";

  Value root;
  root["consumerIdList"] = value;

  FastWriter writer;
  std::string data = writer.write(root);

  const ByteArray bodyData((char*)data.data(), data.size());
  std::unique_ptr<GetConsumerListByGroupResponseBody> body(GetConsumerListByGroupResponseBody::Decode(bodyData));
  EXPECT_EQ(body->consumerIdList.size(), 2);
}

TEST(CommandHeaderTest, ResetOffsetRequestHeader) {
  ResetOffsetRequestHeader header;

  header.setTopic("testTopic");
  EXPECT_EQ(header.getTopic(), "testTopic");

  header.setGroup("testGroup");
  EXPECT_EQ(header.getGroup(), "testGroup");

  header.setTimeStamp(123);
  EXPECT_EQ(header.getTimeStamp(), 123);

  header.setForceFlag(true);
  EXPECT_TRUE(header.getForceFlag());

  std::map<std::string, std::string> resetOffsetFields;
  resetOffsetFields["topic"] = "testTopic";
  resetOffsetFields["group"] = "testGroup";
  resetOffsetFields["timestamp"] = "123";
  resetOffsetFields["isForce"] = "true";
  std::unique_ptr<ResetOffsetRequestHeader> resetOffsetHeader(ResetOffsetRequestHeader::Decode(resetOffsetFields));
  EXPECT_EQ(resetOffsetHeader->getTopic(), "testTopic");
  EXPECT_EQ(resetOffsetHeader->getGroup(), "testGroup");
  EXPECT_EQ(resetOffsetHeader->getTimeStamp(), 123);
  EXPECT_TRUE(resetOffsetHeader->getForceFlag());
}

TEST(CommandHeaderTest, GetConsumerRunningInfoRequestHeader) {
  GetConsumerRunningInfoRequestHeader header;
  header.setClientId("testClientId");
  header.setConsumerGroup("testConsumer");
  header.setJstackEnable(true);

  std::map<std::string, std::string> requestMap;
  header.SetDeclaredFieldOfCommandHeader(requestMap);
  EXPECT_EQ(requestMap["clientId"], "testClientId");
  EXPECT_EQ(requestMap["consumerGroup"], "testConsumer");
  EXPECT_EQ(requestMap["jstackEnable"], "true");

  Value outData;
  header.Encode(outData);
  EXPECT_EQ(outData["clientId"], "testClientId");
  EXPECT_EQ(outData["consumerGroup"], "testConsumer");
  EXPECT_EQ(outData["jstackEnable"], "true");

  std::unique_ptr<GetConsumerRunningInfoRequestHeader> decodeHeader(
      GetConsumerRunningInfoRequestHeader::Decode(requestMap));
  EXPECT_EQ(decodeHeader->getClientId(), "testClientId");
  EXPECT_EQ(decodeHeader->getConsumerGroup(), "testConsumer");
  EXPECT_TRUE(decodeHeader->isJstackEnable());
}

TEST(CommandHeaderTest, NotifyConsumerIdsChangedRequestHeader) {
  std::map<std::string, std::string> extFields;
  extFields["consumerGroup"] = "testGroup";
  std::unique_ptr<NotifyConsumerIdsChangedRequestHeader> header(
      NotifyConsumerIdsChangedRequestHeader::Decode(extFields));
  EXPECT_EQ(header->getConsumerGroup(), "testGroup");
}

int main(int argc, char* argv[]) {
  InitGoogleMock(&argc, argv);
  testing::GTEST_FLAG(throw_on_failure) = true;
  testing::GTEST_FLAG(filter) = "CommandHeaderTest.*";
  return RUN_ALL_TESTS();
}
