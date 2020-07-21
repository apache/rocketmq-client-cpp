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
#include <string>

#include "ByteArray.h"
#include "MQProtos.h"
#include "MQVersion.h"
#include "RemotingCommand.h"
#include "protocol/header/CommandHeader.h"

using testing::InitGoogleMock;
using testing::InitGoogleTest;
using testing::Return;

using rocketmq::ByteArray;
using rocketmq::GetConsumerRunningInfoRequestHeader;
using rocketmq::GetEarliestMsgStoretimeResponseHeader;
using rocketmq::GetMaxOffsetResponseHeader;
using rocketmq::GetMinOffsetResponseHeader;
using rocketmq::GetRouteInfoRequestHeader;
using rocketmq::MQRequestCode;
using rocketmq::MQVersion;
using rocketmq::NotifyConsumerIdsChangedRequestHeader;
using rocketmq::PullMessageResponseHeader;
using rocketmq::QueryConsumerOffsetResponseHeader;
using rocketmq::RemotingCommand;
using rocketmq::ResetOffsetRequestHeader;
using rocketmq::SearchOffsetResponseHeader;
using rocketmq::SendMessageResponseHeader;

TEST(RemotingCommandTest, Init) {
  RemotingCommand remotingCommand;
  EXPECT_EQ(remotingCommand.code(), 0);

  RemotingCommand twoRemotingCommand(13);
  EXPECT_EQ(twoRemotingCommand.code(), 13);
  EXPECT_EQ(twoRemotingCommand.opaque(), 0);
  EXPECT_EQ(twoRemotingCommand.remark(), "");
  EXPECT_EQ(twoRemotingCommand.version(), MQVersion::CURRENT_VERSION);
  EXPECT_EQ(twoRemotingCommand.flag(), 0);
  EXPECT_TRUE(twoRemotingCommand.body() == nullptr);
  EXPECT_TRUE(twoRemotingCommand.readCustomHeader() == nullptr);

  RemotingCommand threeRemotingCommand(13, new GetRouteInfoRequestHeader("topic"));
  EXPECT_FALSE(threeRemotingCommand.readCustomHeader() == nullptr);

  RemotingCommand frouRemotingCommand(13, MQVersion::CURRENT_LANGUAGE, MQVersion::CURRENT_VERSION, 12, 3, "remark",
                                      new GetRouteInfoRequestHeader("topic"));
  EXPECT_EQ(frouRemotingCommand.code(), 13);
  EXPECT_EQ(frouRemotingCommand.opaque(), 12);
  EXPECT_EQ(frouRemotingCommand.remark(), "remark");
  EXPECT_EQ(frouRemotingCommand.version(), MQVersion::CURRENT_VERSION);
  EXPECT_EQ(frouRemotingCommand.flag(), 3);
  EXPECT_TRUE(frouRemotingCommand.body() == nullptr);
  EXPECT_FALSE(frouRemotingCommand.readCustomHeader() == nullptr);

  RemotingCommand sixRemotingCommand(std::move(frouRemotingCommand));
  EXPECT_EQ(sixRemotingCommand.code(), 13);
  EXPECT_EQ(sixRemotingCommand.opaque(), 12);
  EXPECT_EQ(sixRemotingCommand.remark(), "remark");
  EXPECT_EQ(sixRemotingCommand.version(), MQVersion::CURRENT_VERSION);
  EXPECT_EQ(sixRemotingCommand.flag(), 3);
  EXPECT_TRUE(sixRemotingCommand.body() == nullptr);
  EXPECT_FALSE(sixRemotingCommand.readCustomHeader() == nullptr);

  RemotingCommand* sevenRemotingCommand = &sixRemotingCommand;
  EXPECT_EQ(sevenRemotingCommand->code(), 13);
  EXPECT_EQ(sevenRemotingCommand->opaque(), 12);
  EXPECT_EQ(sevenRemotingCommand->remark(), "remark");
  EXPECT_EQ(sevenRemotingCommand->version(), MQVersion::CURRENT_VERSION);
  EXPECT_EQ(sevenRemotingCommand->flag(), 3);
  EXPECT_TRUE(sevenRemotingCommand->body() == nullptr);
  EXPECT_FALSE(sevenRemotingCommand->readCustomHeader() == nullptr);
}

TEST(RemotingCommandTest, Info) {
  RemotingCommand remotingCommand;

  remotingCommand.set_code(13);
  EXPECT_EQ(remotingCommand.code(), 13);

  remotingCommand.set_opaque(12);
  EXPECT_EQ(remotingCommand.opaque(), 12);

  remotingCommand.set_remark("123");
  EXPECT_EQ(remotingCommand.remark(), "123");

  remotingCommand.set_body("msgBody");
  EXPECT_EQ((std::string)remotingCommand.body()->array(), "msgBody");

  remotingCommand.set_ext_field("key", "value");
}

TEST(RemotingCommandTest, Flag) {
  RemotingCommand remotingCommand(13, MQVersion::CURRENT_LANGUAGE, MQVersion::CURRENT_VERSION, 12, 0, "remark",
                                  new GetRouteInfoRequestHeader("topic"));
  ;
  EXPECT_EQ(remotingCommand.flag(), 0);

  remotingCommand.markResponseType();
  int bits = 1 << 0;
  int flag = 0;
  flag |= bits;
  EXPECT_EQ(remotingCommand.flag(), flag);
  EXPECT_TRUE(remotingCommand.isResponseType());

  bits = 1 << 1;
  flag |= bits;
  remotingCommand.markOnewayRPC();
  EXPECT_EQ(remotingCommand.flag(), flag);
  EXPECT_TRUE(remotingCommand.isOnewayRPC());
}

TEST(RemotingCommandTest, EncodeAndDecode) {
  RemotingCommand remotingCommand(MQRequestCode::QUERY_BROKER_OFFSET, MQVersion::CURRENT_LANGUAGE,
                                  MQVersion::CURRENT_VERSION, 12, 3, "remark", nullptr);
  remotingCommand.set_body("123123");

  auto package = remotingCommand.encode();

  std::unique_ptr<RemotingCommand> decodeRemtingCommand(RemotingCommand::Decode(package, true));

  EXPECT_EQ(remotingCommand.code(), decodeRemtingCommand->code());
  EXPECT_EQ(remotingCommand.opaque(), decodeRemtingCommand->opaque());
  EXPECT_EQ(remotingCommand.remark(), decodeRemtingCommand->remark());
  EXPECT_EQ(remotingCommand.version(), decodeRemtingCommand->version());
  EXPECT_EQ(remotingCommand.flag(), decodeRemtingCommand->flag());
  EXPECT_TRUE(decodeRemtingCommand->readCustomHeader() == nullptr);

  // ~RemotingCommand delete
  GetConsumerRunningInfoRequestHeader* requestHeader = new GetConsumerRunningInfoRequestHeader();
  requestHeader->setClientId("client");
  requestHeader->setConsumerGroup("consumerGroup");
  requestHeader->setJstackEnable(false);

  RemotingCommand remotingCommand2(MQRequestCode::GET_CONSUMER_RUNNING_INFO, MQVersion::CURRENT_LANGUAGE,
                                   MQVersion::CURRENT_VERSION, 12, 3, "remark", requestHeader);
  remotingCommand2.set_body("123123");
  package = remotingCommand2.encode();

  decodeRemtingCommand.reset(RemotingCommand::Decode(package, true));

  auto* header = decodeRemtingCommand->decodeCommandCustomHeader<GetConsumerRunningInfoRequestHeader>();
  EXPECT_EQ(requestHeader->getClientId(), header->getClientId());
  EXPECT_EQ(requestHeader->getConsumerGroup(), header->getConsumerGroup());
}

TEST(RemotingCommandTest, SetExtHeader) {
  std::unique_ptr<RemotingCommand> remotingCommand(new RemotingCommand());

  EXPECT_TRUE(remotingCommand->readCustomHeader() == nullptr);

  remotingCommand->set_ext_field("msgId", "ABCD");
  remotingCommand->set_ext_field("queueId", "1");
  remotingCommand->set_ext_field("queueOffset", "1024");
  auto* sendMessageResponseHeader = remotingCommand->decodeCommandCustomHeader<SendMessageResponseHeader>();
  EXPECT_EQ(sendMessageResponseHeader->msgId, "ABCD");
  EXPECT_EQ(sendMessageResponseHeader->queueId, 1);
  EXPECT_EQ(sendMessageResponseHeader->queueOffset, 1024);
}

int main(int argc, char* argv[]) {
  InitGoogleMock(&argc, argv);
  testing::GTEST_FLAG(throw_on_failure) = true;
  testing::GTEST_FLAG(filter) = "RemotingCommandTest.*";
  return RUN_ALL_TESTS();
}
