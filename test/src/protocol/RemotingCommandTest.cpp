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

#include "DataBlock.h"
#include "MQProtos.h"
#include "MQVersion.h"
#include "MemoryOutputStream.h"
#include "RemotingCommand.h"
#include "protocol/header/CommandHeader.h"

using testing::InitGoogleMock;
using testing::InitGoogleTest;
using testing::Return;

using rocketmq::GetConsumerRunningInfoRequestHeader;
using rocketmq::GetEarliestMsgStoretimeResponseHeader;
using rocketmq::GetMaxOffsetResponseHeader;
using rocketmq::GetMinOffsetResponseHeader;
using rocketmq::GetRouteInfoRequestHeader;
using rocketmq::MemoryBlock;
using rocketmq::MemoryOutputStream;
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
  EXPECT_EQ(remotingCommand.getCode(), 0);

  RemotingCommand twoRemotingCommand(13);
  EXPECT_EQ(twoRemotingCommand.getCode(), 13);
  EXPECT_EQ(twoRemotingCommand.getOpaque(), 0);
  EXPECT_EQ(twoRemotingCommand.getRemark(), "");
  EXPECT_EQ(twoRemotingCommand.getVersion(), MQVersion::s_CurrentVersion);
  EXPECT_EQ(twoRemotingCommand.getFlag(), 0);
  EXPECT_TRUE(twoRemotingCommand.getBody() == nullptr);
  EXPECT_TRUE(twoRemotingCommand.readCustomHeader() == nullptr);

  RemotingCommand threeRemotingCommand(13, new GetRouteInfoRequestHeader("topic"));
  EXPECT_FALSE(threeRemotingCommand.readCustomHeader() == nullptr);

  RemotingCommand frouRemotingCommand(13, "CPP", MQVersion::s_CurrentVersion, 12, 3, "remark",
                                      new GetRouteInfoRequestHeader("topic"));
  EXPECT_EQ(frouRemotingCommand.getCode(), 13);
  EXPECT_EQ(frouRemotingCommand.getOpaque(), 12);
  EXPECT_EQ(frouRemotingCommand.getRemark(), "remark");
  EXPECT_EQ(frouRemotingCommand.getVersion(), MQVersion::s_CurrentVersion);
  EXPECT_EQ(frouRemotingCommand.getFlag(), 3);
  EXPECT_TRUE(frouRemotingCommand.getBody() == nullptr);
  EXPECT_FALSE(frouRemotingCommand.readCustomHeader() == nullptr);

  RemotingCommand sixRemotingCommand(std::move(frouRemotingCommand));
  EXPECT_EQ(sixRemotingCommand.getCode(), 13);
  EXPECT_EQ(sixRemotingCommand.getOpaque(), 12);
  EXPECT_EQ(sixRemotingCommand.getRemark(), "remark");
  EXPECT_EQ(sixRemotingCommand.getVersion(), MQVersion::s_CurrentVersion);
  EXPECT_EQ(sixRemotingCommand.getFlag(), 3);
  EXPECT_TRUE(sixRemotingCommand.getBody() == nullptr);
  EXPECT_FALSE(sixRemotingCommand.readCustomHeader() == nullptr);

  RemotingCommand* sevenRemotingCommand = &sixRemotingCommand;
  EXPECT_EQ(sevenRemotingCommand->getCode(), 13);
  EXPECT_EQ(sevenRemotingCommand->getOpaque(), 12);
  EXPECT_EQ(sevenRemotingCommand->getRemark(), "remark");
  EXPECT_EQ(sevenRemotingCommand->getVersion(), MQVersion::s_CurrentVersion);
  EXPECT_EQ(sevenRemotingCommand->getFlag(), 3);
  EXPECT_TRUE(sevenRemotingCommand->getBody() == nullptr);
  EXPECT_FALSE(sevenRemotingCommand->readCustomHeader() == nullptr);
}

TEST(RemotingCommandTest, Info) {
  RemotingCommand remotingCommand;

  remotingCommand.setCode(13);
  EXPECT_EQ(remotingCommand.getCode(), 13);

  remotingCommand.setOpaque(12);
  EXPECT_EQ(remotingCommand.getOpaque(), 12);

  remotingCommand.setRemark("123");
  EXPECT_EQ(remotingCommand.getRemark(), "123");

  remotingCommand.setBody("msgBody");
  EXPECT_EQ((std::string)*remotingCommand.getBody(), "msgBody");

  remotingCommand.addExtField("key", "value");
}

TEST(RemotingCommandTest, Flag) {
  RemotingCommand remotingCommand(13, "CPP", MQVersion::s_CurrentVersion, 12, 0, "remark",
                                  new GetRouteInfoRequestHeader("topic"));
  ;
  EXPECT_EQ(remotingCommand.getFlag(), 0);

  remotingCommand.markResponseType();
  int bits = 1 << 0;
  int flag = 0;
  flag |= bits;
  EXPECT_EQ(remotingCommand.getFlag(), flag);
  EXPECT_TRUE(remotingCommand.isResponseType());

  bits = 1 << 1;
  flag |= bits;
  remotingCommand.markOnewayRPC();
  EXPECT_EQ(remotingCommand.getFlag(), flag);
  EXPECT_TRUE(remotingCommand.isOnewayRPC());
}

TEST(RemotingCommandTest, EncodeAndDecode) {
  RemotingCommand remotingCommand(MQRequestCode::QUERY_BROKER_OFFSET, "CPP", MQVersion::s_CurrentVersion, 12, 3,
                                  "remark", nullptr);
  remotingCommand.setBody("123123");

  auto package = remotingCommand.encode();

  std::unique_ptr<RemotingCommand> decodeRemtingCommand(
      RemotingCommand::Decode(std::shared_ptr<MemoryBlock>(std::move(package)), true));

  EXPECT_EQ(remotingCommand.getCode(), decodeRemtingCommand->getCode());
  EXPECT_EQ(remotingCommand.getOpaque(), decodeRemtingCommand->getOpaque());
  EXPECT_EQ(remotingCommand.getRemark(), decodeRemtingCommand->getRemark());
  EXPECT_EQ(remotingCommand.getVersion(), decodeRemtingCommand->getVersion());
  EXPECT_EQ(remotingCommand.getFlag(), decodeRemtingCommand->getFlag());
  EXPECT_TRUE(decodeRemtingCommand->readCustomHeader() == nullptr);

  // ~RemotingCommand delete
  GetConsumerRunningInfoRequestHeader* requestHeader = new GetConsumerRunningInfoRequestHeader();
  requestHeader->setClientId("client");
  requestHeader->setConsumerGroup("consumerGroup");
  requestHeader->setJstackEnable(false);

  RemotingCommand remotingCommand2(MQRequestCode::GET_CONSUMER_RUNNING_INFO, "CPP", MQVersion::s_CurrentVersion, 12, 3,
                                   "remark", requestHeader);
  remotingCommand2.setBody("123123");
  package = remotingCommand2.encode();

  decodeRemtingCommand.reset(RemotingCommand::Decode(std::shared_ptr<MemoryBlock>(std::move(package)), true));

  auto* header = decodeRemtingCommand->decodeCommandCustomHeader<GetConsumerRunningInfoRequestHeader>();
  EXPECT_EQ(requestHeader->getClientId(), header->getClientId());
  EXPECT_EQ(requestHeader->getConsumerGroup(), header->getConsumerGroup());
}

TEST(RemotingCommandTest, SetExtHeader) {
  std::unique_ptr<RemotingCommand> remotingCommand(new RemotingCommand());

  EXPECT_TRUE(remotingCommand->readCustomHeader() == nullptr);

  remotingCommand->addExtField("msgId", "ABCD");
  remotingCommand->addExtField("queueId", "1");
  remotingCommand->addExtField("queueOffset", "1024");
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
