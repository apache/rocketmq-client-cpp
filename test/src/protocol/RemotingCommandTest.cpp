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

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "RemotingCommand.h"
#include "CommandHeader.h"
#include "MQVersion.h"
#include "dataBlock.h"
#include "MemoryOutputStream.h"
#include "MQProtos.h"

using ::testing::InitGoogleTest;
using ::testing::InitGoogleMock;
using testing::Return;

using rocketmq::RemotingCommand;
using rocketmq::MQVersion;
using rocketmq::CommandHeader;
using rocketmq::GetRouteInfoRequestHeader;
using rocketmq::GetConsumerRunningInfoRequestHeader;
using rocketmq::MemoryBlock;
using rocketmq::MemoryOutputStream;
using rocketmq::MQRequestCode;
using rocketmq::SendMessageResponseHeader;
using rocketmq::PullMessageResponseHeader;
using rocketmq::GetMinOffsetResponseHeader;
using rocketmq::GetMaxOffsetResponseHeader;
using rocketmq::SearchOffsetResponseHeader;
using rocketmq::GetEarliestMsgStoretimeResponseHeader;
using rocketmq::QueryConsumerOffsetResponseHeader;
using rocketmq::ResetOffsetRequestHeader;
using rocketmq::GetConsumerRunningInfoRequestHeader;
using rocketmq::NotifyConsumerIdsChangedRequestHeader;

TEST(remotingCommand, init) {
    RemotingCommand remotingCommand;
    EXPECT_EQ(remotingCommand.getCode(), 0);
    EXPECT_EQ(remotingCommand.getOpaque(), 0);
    EXPECT_EQ(remotingCommand.getRemark(), "");
    EXPECT_EQ(remotingCommand.getVersion(), 0);
    //  EXPECT_EQ(remotingCommand.getFlag() , 0);
    EXPECT_EQ(remotingCommand.getMsgBody(), "");
    EXPECT_TRUE(remotingCommand.getCommandHeader() == NULL);

    RemotingCommand twoRemotingCommand(13);
    EXPECT_EQ(twoRemotingCommand.getCode(), 13);
    EXPECT_EQ(twoRemotingCommand.getOpaque(), 0);
    EXPECT_EQ(twoRemotingCommand.getRemark(), "");
    EXPECT_EQ(twoRemotingCommand.getVersion(), MQVersion::s_CurrentVersion);
    //EXPECT_EQ(twoRemotingCommand.getFlag() , 0);
    EXPECT_EQ(twoRemotingCommand.getMsgBody(), "");
    EXPECT_TRUE(twoRemotingCommand.getCommandHeader() == NULL);

    RemotingCommand threeRemotingCommand(
            13, new GetRouteInfoRequestHeader("topic"));
    EXPECT_FALSE(threeRemotingCommand.getCommandHeader() == NULL);

    RemotingCommand frouRemotingCommand(13, "CPP", MQVersion::s_CurrentVersion,
                                        12, 3, "remark",
                                        new GetRouteInfoRequestHeader("topic"));
    EXPECT_EQ(frouRemotingCommand.getCode(), 13);
    EXPECT_EQ(frouRemotingCommand.getOpaque(), 12);
    EXPECT_EQ(frouRemotingCommand.getRemark(), "remark");
    EXPECT_EQ(frouRemotingCommand.getVersion(), MQVersion::s_CurrentVersion);
    EXPECT_EQ(frouRemotingCommand.getFlag(), 3);
    EXPECT_EQ(frouRemotingCommand.getMsgBody(), "");
    EXPECT_FALSE(frouRemotingCommand.getCommandHeader() == NULL);

    RemotingCommand sixRemotingCommand(frouRemotingCommand);
    EXPECT_EQ(sixRemotingCommand.getCode(), 13);
    EXPECT_EQ(sixRemotingCommand.getOpaque(), 12);
    EXPECT_EQ(sixRemotingCommand.getRemark(), "remark");
    EXPECT_EQ(sixRemotingCommand.getVersion(), MQVersion::s_CurrentVersion);
    EXPECT_EQ(sixRemotingCommand.getFlag(), 3);
    EXPECT_EQ(sixRemotingCommand.getMsgBody(), "");
    EXPECT_TRUE(sixRemotingCommand.getCommandHeader() == NULL);

    RemotingCommand* sevenRemotingCommand = new RemotingCommand();
    EXPECT_EQ(sevenRemotingCommand->getCode(), 0);
    EXPECT_EQ(sevenRemotingCommand->getOpaque(), 0);
    EXPECT_EQ(sevenRemotingCommand->getRemark(), "");
    EXPECT_EQ(sevenRemotingCommand->getVersion(), 0);
    EXPECT_EQ(sevenRemotingCommand->getFlag(), 0);
    EXPECT_EQ(sevenRemotingCommand->getMsgBody(), "");
    EXPECT_TRUE(sevenRemotingCommand->getCommandHeader() == NULL);

    RemotingCommand* egthRemotingCommand = sevenRemotingCommand;
    EXPECT_EQ(egthRemotingCommand->getCode(), 0);
    EXPECT_EQ(egthRemotingCommand->getOpaque(), 0);
    EXPECT_EQ(egthRemotingCommand->getRemark(), "");
    EXPECT_EQ(egthRemotingCommand->getVersion(), 0);
    EXPECT_EQ(egthRemotingCommand->getFlag(), 0);
    EXPECT_EQ(egthRemotingCommand->getMsgBody(), "");
    EXPECT_TRUE(egthRemotingCommand->getCommandHeader() == NULL);

    sevenRemotingCommand = &sixRemotingCommand;
    EXPECT_EQ(sevenRemotingCommand->getCode(), 13);
    EXPECT_EQ(sevenRemotingCommand->getOpaque(), 12);
    EXPECT_EQ(sevenRemotingCommand->getRemark(), "remark");
    EXPECT_EQ(sevenRemotingCommand->getVersion(), MQVersion::s_CurrentVersion);
    EXPECT_EQ(sevenRemotingCommand->getFlag(), 3);
    EXPECT_EQ(sevenRemotingCommand->getMsgBody(), "");
    EXPECT_TRUE(sevenRemotingCommand->getCommandHeader() == NULL);

    // Assign
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
    RemotingCommand remotingCommand(13, "CPP", MQVersion::s_CurrentVersion, 12,
                                    0, "remark",
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

TEST(remotingCommand , encodeAndDecode) {
    RemotingCommand remotingCommand(13, "CPP", MQVersion::s_CurrentVersion, 12,
                                    3, "remark", NULL);
    remotingCommand.SetBody("123123", 6);
    remotingCommand.Encode();
    const MemoryBlock* head = remotingCommand.GetHead();
    const MemoryBlock* body = remotingCommand.GetBody();

    unique_ptr<MemoryOutputStream> result(new MemoryOutputStream(1024));
    result->write(head->getData() + 4, head->getSize() - 4);
    result->write(body->getData(), body->getSize());

    RemotingCommand* decodeRemtingCommand = RemotingCommand::Decode(
            result->getMemoryBlock());
    EXPECT_EQ(remotingCommand.getCode(), decodeRemtingCommand->getCode());
    EXPECT_EQ(remotingCommand.getOpaque(), decodeRemtingCommand->getOpaque());
    EXPECT_EQ(remotingCommand.getRemark(), decodeRemtingCommand->getRemark());
    EXPECT_EQ(remotingCommand.getVersion(), decodeRemtingCommand->getVersion());
    EXPECT_EQ(remotingCommand.getFlag(), decodeRemtingCommand->getFlag());
    EXPECT_TRUE(decodeRemtingCommand->getCommandHeader() == NULL);
    delete decodeRemtingCommand;

    GetConsumerRunningInfoRequestHeader* requestHeader =
            new GetConsumerRunningInfoRequestHeader();
    requestHeader->setClientId("client");
    requestHeader->setConsumerGroup("consumerGroup");
    requestHeader->setJstackEnable(false);

    RemotingCommand encodeRemotingCommand(307, "CPP",
                                          MQVersion::s_CurrentVersion, 12, 3,
                                          "remark", requestHeader);
    encodeRemotingCommand.SetBody("123123", 6);
    encodeRemotingCommand.Encode();
    const MemoryBlock* phead = encodeRemotingCommand.GetHead();
    const MemoryBlock* pbody = encodeRemotingCommand.GetBody();

    unique_ptr<MemoryOutputStream> results(new MemoryOutputStream(1024));
    results->write(phead->getData() + 4, phead->getSize() - 4);
    results->write(pbody->getData(), pbody->getSize());

    decodeRemtingCommand = RemotingCommand::Decode(results->getMemoryBlock());

    decodeRemtingCommand->SetExtHeader(decodeRemtingCommand->getCode());
    GetConsumerRunningInfoRequestHeader* header =
            (GetConsumerRunningInfoRequestHeader*) decodeRemtingCommand
                    ->getCommandHeader();
    EXPECT_EQ(requestHeader->getClientId(), header->getClientId());
    EXPECT_EQ(requestHeader->getConsumerGroup(), header->getConsumerGroup());
    //EXPECT_EQ(requestHeader->isJstackEnable(), header->isJstackEnable());

}

TEST(remotingCommand, SetExtHeader) {
    RemotingCommand* remotingCommand = new RemotingCommand();

    remotingCommand->SetExtHeader(-1);
    EXPECT_TRUE(remotingCommand->getCommandHeader() == NULL);

    Json::Value object;
    Json::Value extFields;
    object["extFields"] = extFields;
    remotingCommand->setParsedJson(object);

    remotingCommand->SetExtHeader(MQRequestCode::SEND_MESSAGE);
    SendMessageResponseHeader* sendMessageResponseHeader =
            (SendMessageResponseHeader*) remotingCommand->getCommandHeader();
    sendMessageResponseHeader->msgId;

    remotingCommand->SetExtHeader(MQRequestCode::PULL_MESSAGE);
    PullMessageResponseHeader* pullMessageResponseHeader =
            (PullMessageResponseHeader*) remotingCommand->getCommandHeader();
    pullMessageResponseHeader->suggestWhichBrokerId;

    remotingCommand->SetExtHeader(MQRequestCode::GET_MIN_OFFSET);
    GetMinOffsetResponseHeader* getMinOffsetResponseHeader =
            (GetMinOffsetResponseHeader*) remotingCommand->getCommandHeader();
    getMinOffsetResponseHeader->offset;

    remotingCommand->SetExtHeader(MQRequestCode::GET_MAX_OFFSET);
    GetMaxOffsetResponseHeader* getMaxOffsetResponseHeader =
            (GetMaxOffsetResponseHeader*) remotingCommand->getCommandHeader();
    getMaxOffsetResponseHeader->offset;

    remotingCommand->SetExtHeader(MQRequestCode::SEARCH_OFFSET_BY_TIMESTAMP);
    SearchOffsetResponseHeader* searchOffsetResponseHeader =
            (SearchOffsetResponseHeader*) remotingCommand->getCommandHeader();
    searchOffsetResponseHeader->offset;

    remotingCommand->SetExtHeader(MQRequestCode::GET_EARLIEST_MSG_STORETIME);
    GetEarliestMsgStoretimeResponseHeader* getEarliestMsgStoretimeResponseHeader =
            (GetEarliestMsgStoretimeResponseHeader*) remotingCommand
                    ->getCommandHeader();
    getEarliestMsgStoretimeResponseHeader->timestamp;

    remotingCommand->SetExtHeader(MQRequestCode::QUERY_CONSUMER_OFFSET);
    QueryConsumerOffsetResponseHeader* queryConsumerOffsetResponseHeader =
            (QueryConsumerOffsetResponseHeader*) remotingCommand
                    ->getCommandHeader();
    queryConsumerOffsetResponseHeader->offset;

    remotingCommand->SetExtHeader(MQRequestCode::RESET_CONSUMER_CLIENT_OFFSET);
    ResetOffsetRequestHeader* resetOffsetRequestHeader =
            (ResetOffsetRequestHeader*) remotingCommand->getCommandHeader();
    //resetOffsetRequestHeader->setGroup("group");

    remotingCommand->SetExtHeader(MQRequestCode::GET_CONSUMER_RUNNING_INFO);
    GetConsumerRunningInfoRequestHeader* getConsumerRunningInfoRequestHeader =
            (GetConsumerRunningInfoRequestHeader*) remotingCommand
                    ->getCommandHeader();
    //getConsumerRunningInfoRequestHeader->setClientId("id");

    remotingCommand->SetExtHeader(MQRequestCode::NOTIFY_CONSUMER_IDS_CHANGED);
    NotifyConsumerIdsChangedRequestHeader* notifyConsumerIdsChangedRequestHeader =
            (NotifyConsumerIdsChangedRequestHeader*) remotingCommand
                    ->getCommandHeader();
    //notifyConsumerIdsChangedRequestHeader->setGroup("group");
}

int main(int argc, char* argv[]) {
    InitGoogleMock(&argc, argv);
    testing::GTEST_FLAG(throw_on_failure) = true;
    testing::GTEST_FLAG(filter) = "remotingCommand.*";
    int itestts = RUN_ALL_TESTS();
    return itestts;
}
