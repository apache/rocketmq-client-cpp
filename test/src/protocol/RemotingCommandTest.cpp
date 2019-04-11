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

#include <stdlib.h>
#include <iostream>
#include <memory>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "CommandHeader.h"
#include "MQProtos.h"
#include "MQVersion.h"
#include "MemoryOutputStream.h"
#include "RemotingCommand.h"
#include "dataBlock.h"

using std::shared_ptr;

using ::testing::InitGoogleMock;
using ::testing::InitGoogleTest;
using testing::Return;

using rocketmq::CommandHeader;
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

TEST(remotingCommand, init) {
  RemotingCommand remotingCommand;
  EXPECT_EQ(remotingCommand.getCode(), 0);

  RemotingCommand twoRemotingCommand(13);
  EXPECT_EQ(twoRemotingCommand.getCode(), 13);
  EXPECT_EQ(twoRemotingCommand.getOpaque(), 0);
  EXPECT_EQ(twoRemotingCommand.getRemark(), "");
  EXPECT_EQ(twoRemotingCommand.getVersion(), MQVersion::s_CurrentVersion);
  // EXPECT_EQ(twoRemotingCommand.getFlag() , 0);
  EXPECT_EQ(twoRemotingCommand.getMsgBody(), "");
  EXPECT_TRUE(twoRemotingCommand.getCommandHeader() == nullptr);

  RemotingCommand threeRemotingCommand(13, new GetRouteInfoRequestHeader("topic"));
  EXPECT_FALSE(threeRemotingCommand.getCommandHeader() == nullptr);

  RemotingCommand frouRemotingCommand(13, "CPP", MQVersion::s_CurrentVersion, 12, 3, "remark",
                                      new GetRouteInfoRequestHeader("topic"));
  EXPECT_EQ(frouRemotingCommand.getCode(), 13);
  EXPECT_EQ(frouRemotingCommand.getOpaque(), 12);
  EXPECT_EQ(frouRemotingCommand.getRemark(), "remark");
  EXPECT_EQ(frouRemotingCommand.getVersion(), MQVersion::s_CurrentVersion);
  EXPECT_EQ(frouRemotingCommand.getFlag(), 3);
  EXPECT_EQ(frouRemotingCommand.getMsgBody(), "");
  EXPECT_FALSE(frouRemotingCommand.getCommandHeader() == nullptr);

  RemotingCommand sixRemotingCommand(frouRemotingCommand);
  EXPECT_EQ(sixRemotingCommand.getCode(), 13);
  EXPECT_EQ(sixRemotingCommand.getOpaque(), 12);
  EXPECT_EQ(sixRemotingCommand.getRemark(), "remark");
  EXPECT_EQ(sixRemotingCommand.getVersion(), MQVersion::s_CurrentVersion);
  EXPECT_EQ(sixRemotingCommand.getFlag(), 3);
  EXPECT_EQ(sixRemotingCommand.getMsgBody(), "");
  EXPECT_TRUE(sixRemotingCommand.getCommandHeader() == nullptr);

  RemotingCommand* sevenRemotingCommand = &sixRemotingCommand;
  EXPECT_EQ(sevenRemotingCommand->getCode(), 13);
  EXPECT_EQ(sevenRemotingCommand->getOpaque(), 12);
  EXPECT_EQ(sevenRemotingCommand->getRemark(), "remark");
  EXPECT_EQ(sevenRemotingCommand->getVersion(), MQVersion::s_CurrentVersion);
  EXPECT_EQ(sevenRemotingCommand->getFlag(), 3);
  EXPECT_EQ(sevenRemotingCommand->getMsgBody(), "");
  EXPECT_TRUE(sevenRemotingCommand->getCommandHeader() == nullptr);
}

TEST(remotingCommand, info) {
  RemotingCommand remotingCommand;

  remotingCommand.setCode(13);
  EXPECT_EQ(remotingCommand.getCode(), 13);

  remotingCommand.setOpaque(12);
  EXPECT_EQ(remotingCommand.getOpaque(), 12);

  remotingCommand.setRemark("123");
  EXPECT_EQ(remotingCommand.getRemark(), "123");

  remotingCommand.setMsgBody("msgBody");
  EXPECT_EQ(remotingCommand.getMsgBody(), "msgBody");

  remotingCommand.addExtField("key", "value");
}

TEST(remotingCommand, flag) {
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

TEST(remotingCommand, encodeAndDecode) {
  RemotingCommand remotingCommand(13, "CPP", MQVersion::s_CurrentVersion, 12, 3, "remark", NULL);
  remotingCommand.SetBody("123123", 6);
  remotingCommand.Encode();
  // no delete
  const MemoryBlock* head = remotingCommand.GetHead();
  const MemoryBlock* body = remotingCommand.GetBody();

  unique_ptr<MemoryOutputStream> result(new MemoryOutputStream(1024));
  result->write(head->getData() + 4, head->getSize() - 4);
  result->write(body->getData(), body->getSize());

  shared_ptr<RemotingCommand> decodeRemtingCommand(RemotingCommand::Decode(result->getMemoryBlock()));
  EXPECT_EQ(remotingCommand.getCode(), decodeRemtingCommand->getCode());
  EXPECT_EQ(remotingCommand.getOpaque(), decodeRemtingCommand->getOpaque());
  EXPECT_EQ(remotingCommand.getRemark(), decodeRemtingCommand->getRemark());
  EXPECT_EQ(remotingCommand.getVersion(), decodeRemtingCommand->getVersion());
  EXPECT_EQ(remotingCommand.getFlag(), decodeRemtingCommand->getFlag());
  EXPECT_TRUE(decodeRemtingCommand->getCommandHeader() == NULL);

  // ~RemotingCommand delete
  GetConsumerRunningInfoRequestHeader* requestHeader = new GetConsumerRunningInfoRequestHeader();
  requestHeader->setClientId("client");
  requestHeader->setConsumerGroup("consumerGroup");
  requestHeader->setJstackEnable(false);

  RemotingCommand encodeRemotingCommand(307, "CPP", MQVersion::s_CurrentVersion, 12, 3, "remark", requestHeader);
  encodeRemotingCommand.SetBody("123123", 6);
  encodeRemotingCommand.Encode();

  // no delete
  const MemoryBlock* phead = encodeRemotingCommand.GetHead();
  const MemoryBlock* pbody = encodeRemotingCommand.GetBody();

  unique_ptr<MemoryOutputStream> results(new MemoryOutputStream(1024));
  results->write(phead->getData() + 4, phead->getSize() - 4);
  results->write(pbody->getData(), pbody->getSize());

  shared_ptr<RemotingCommand> decodeRemtingCommandTwo(RemotingCommand::Decode(results->getMemoryBlock()));

  decodeRemtingCommandTwo->SetExtHeader(encodeRemotingCommand.getCode());
  GetConsumerRunningInfoRequestHeader* header =
      reinterpret_cast<GetConsumerRunningInfoRequestHeader*>(decodeRemtingCommandTwo->getCommandHeader());
  EXPECT_EQ(requestHeader->getClientId(), header->getClientId());
  EXPECT_EQ(requestHeader->getConsumerGroup(), header->getConsumerGroup());
}

TEST(remotingCommand, SetExtHeader) {
  shared_ptr<RemotingCommand> remotingCommand(new RemotingCommand());

  remotingCommand->SetExtHeader(-1);
  EXPECT_TRUE(remotingCommand->getCommandHeader() == NULL);

  Json::Value object;
  Json::Value extFields;
  extFields["id"] = 1;
  object["extFields"] = extFields;
  remotingCommand->setParsedJson(object);

  remotingCommand->SetExtHeader(MQRequestCode::SEND_MESSAGE);
  SendMessageResponseHeader* sendMessageResponseHeader =
      reinterpret_cast<SendMessageResponseHeader*>(remotingCommand->getCommandHeader());
  EXPECT_EQ(sendMessageResponseHeader->msgId, "");

  remotingCommand->SetExtHeader(MQRequestCode::PULL_MESSAGE);
  PullMessageResponseHeader* pullMessageResponseHeader =
      reinterpret_cast<PullMessageResponseHeader*>(remotingCommand->getCommandHeader());
  EXPECT_EQ(pullMessageResponseHeader->suggestWhichBrokerId, 0);

  remotingCommand->SetExtHeader(MQRequestCode::GET_MIN_OFFSET);
  GetMinOffsetResponseHeader* getMinOffsetResponseHeader =
      reinterpret_cast<GetMinOffsetResponseHeader*>(remotingCommand->getCommandHeader());
  EXPECT_EQ(getMinOffsetResponseHeader->offset, 0);

  remotingCommand->SetExtHeader(MQRequestCode::GET_MAX_OFFSET);
  GetMaxOffsetResponseHeader* getMaxOffsetResponseHeader =
      reinterpret_cast<GetMaxOffsetResponseHeader*>(remotingCommand->getCommandHeader());
  EXPECT_EQ(getMaxOffsetResponseHeader->offset, 0);

  remotingCommand->SetExtHeader(MQRequestCode::SEARCH_OFFSET_BY_TIMESTAMP);
  SearchOffsetResponseHeader* searchOffsetResponseHeader =
      reinterpret_cast<SearchOffsetResponseHeader*>(remotingCommand->getCommandHeader());
  EXPECT_EQ(searchOffsetResponseHeader->offset, 0);

  remotingCommand->SetExtHeader(MQRequestCode::GET_EARLIEST_MSG_STORETIME);
  GetEarliestMsgStoretimeResponseHeader* getEarliestMsgStoretimeResponseHeader =
      reinterpret_cast<GetEarliestMsgStoretimeResponseHeader*>(remotingCommand->getCommandHeader());
  EXPECT_EQ(getEarliestMsgStoretimeResponseHeader->timestamp, 0);

  remotingCommand->SetExtHeader(MQRequestCode::QUERY_CONSUMER_OFFSET);
  QueryConsumerOffsetResponseHeader* queryConsumerOffsetResponseHeader =
      reinterpret_cast<QueryConsumerOffsetResponseHeader*>(remotingCommand->getCommandHeader());
  EXPECT_EQ(queryConsumerOffsetResponseHeader->offset, 0);

  extFields["isForce"] = "true";
  object["extFields"] = extFields;
  remotingCommand->setParsedJson(object);
  remotingCommand->SetExtHeader(MQRequestCode::RESET_CONSUMER_CLIENT_OFFSET);
  ResetOffsetRequestHeader* resetOffsetRequestHeader =
      reinterpret_cast<ResetOffsetRequestHeader*>(remotingCommand->getCommandHeader());
  resetOffsetRequestHeader->setGroup("group");

  remotingCommand->SetExtHeader(MQRequestCode::GET_CONSUMER_RUNNING_INFO);
  GetConsumerRunningInfoRequestHeader* getConsumerRunningInfoRequestHeader =
      reinterpret_cast<GetConsumerRunningInfoRequestHeader*>(remotingCommand->getCommandHeader());
  getConsumerRunningInfoRequestHeader->setClientId("id");

  remotingCommand->SetExtHeader(MQRequestCode::NOTIFY_CONSUMER_IDS_CHANGED);
  NotifyConsumerIdsChangedRequestHeader* notifyConsumerIdsChangedRequestHeader =
      reinterpret_cast<NotifyConsumerIdsChangedRequestHeader*>(remotingCommand->getCommandHeader());
  notifyConsumerIdsChangedRequestHeader->setGroup("group");
}

int main(int argc, char* argv[]) {
  InitGoogleMock(&argc, argv);
  testing::GTEST_FLAG(throw_on_failure) = true;
  testing::GTEST_FLAG(filter) = "remotingCommand.*";
  int itestts = RUN_ALL_TESTS();
  return itestts;
}
