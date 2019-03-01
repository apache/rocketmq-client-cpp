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
#include <string>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "json/value.h"
#include "json/writer.h"

#include "CommandHeader.h"
#include "MQClientException.h"
#include "MessageSysFlag.h"
#include "UtilAll.h"
#include "dataBlock.h"
#include "json/json.h"

using ::testing::InitGoogleTest;
using ::testing::InitGoogleMock;
using testing::Return;

using Json::Value;
using Json::FastWriter;

using rocketmq::MemoryBlock;
using rocketmq::CommandHeader;
using rocketmq::GetRouteInfoRequestHeader;
using rocketmq::UnregisterClientRequestHeader;
using rocketmq::CreateTopicRequestHeader;
using rocketmq::SendMessageRequestHeader;
using rocketmq::SendMessageResponseHeader;
using rocketmq::PullMessageRequestHeader;
using rocketmq::PullMessageResponseHeader;
using rocketmq::GetConsumerListByGroupResponseHeader;
using rocketmq::GetMinOffsetRequestHeader;
using rocketmq::GetMinOffsetResponseHeader;
using rocketmq::GetMaxOffsetRequestHeader;
using rocketmq::GetMaxOffsetResponseHeader;
using rocketmq::SearchOffsetRequestHeader;
using rocketmq::SearchOffsetResponseHeader;
using rocketmq::ViewMessageRequestHeader;
using rocketmq::GetEarliestMsgStoretimeRequestHeader;
using rocketmq::GetEarliestMsgStoretimeResponseHeader;
using rocketmq::GetConsumerListByGroupRequestHeader;
using rocketmq::QueryConsumerOffsetRequestHeader;
using rocketmq::QueryConsumerOffsetResponseHeader;
using rocketmq::UpdateConsumerOffsetRequestHeader;
using rocketmq::ConsumerSendMsgBackRequestHeader;
using rocketmq::GetConsumerListByGroupResponseBody;
using rocketmq::ResetOffsetRequestHeader;
using rocketmq::GetConsumerRunningInfoRequestHeader;
using rocketmq::NotifyConsumerIdsChangedRequestHeader;

TEST(commandHeader, ConsumerSendMsgBackRequestHeader) {

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
    ResetOffsetRequestHeader::Decode(value);
    EXPECT_EQ(header.getTopic(), "");
    EXPECT_EQ(header.getGroup(), "");
    EXPECT_EQ(header.getTimeStamp(), 0);
    EXPECT_FALSE(header.getForceFlag());

    value["topic"] = "tetTopic";
    ResetOffsetRequestHeader::Decode(value);
    EXPECT_EQ(header.getTopic(), "testTopic");
    EXPECT_EQ(header.getGroup(), "");
    EXPECT_EQ(header.getTimeStamp(), 0);
    EXPECT_FALSE(header.getForceFlag());

    value["group"] = "testGroup";
    ResetOffsetRequestHeader::Decode(value);
    EXPECT_EQ(header.getTopic(), "testTopic");
    EXPECT_EQ(header.getGroup(), "testGroup");
    EXPECT_EQ(header.getTimeStamp(), 0);
    EXPECT_FALSE(header.getForceFlag());

    value["timestamp"] = "123";
    ResetOffsetRequestHeader::Decode(value);
    EXPECT_EQ(header.getTopic(), "testTopic");
    EXPECT_EQ(header.getGroup(), "testGroup");
    EXPECT_EQ(header.getTimeStamp(), 123);
    EXPECT_FALSE(header.getForceFlag());

    value["isForce"] = "true";
    ResetOffsetRequestHeader::Decode(value);
    EXPECT_EQ(header.getTopic(), "testTopic");
    EXPECT_EQ(header.getGroup(), "testGroup");
    EXPECT_EQ(header.getTimeStamp(), 123);
    EXPECT_TRUE(header.getForceFlag());

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
    EXPECT_EQ(outData["jstackEnable"], "true");

    GetConsumerRunningInfoRequestHeader* decodeHeader =
            static_cast<GetConsumerRunningInfoRequestHeader*>(GetConsumerRunningInfoRequestHeader::Decode(
                    outData));
    EXPECT_EQ(decodeHeader->getClientId(), "testConsumer");
    EXPECT_EQ(decodeHeader->getConsumerGroup(), "testClientId");
    EXPECT_TRUE(decodeHeader->isJstackEnable());
    delete decodeHeader;
}

TEST(commandHeader, NotifyConsumerIdsChangedRequestHeader) {
    Json::Value ext;
    NotifyConsumerIdsChangedRequestHeader* header =
            static_cast<NotifyConsumerIdsChangedRequestHeader*>(NotifyConsumerIdsChangedRequestHeader::Decode(
                    ext));
    EXPECT_EQ(header->getGroup(), "");
    delete header;

    ext["consumerGroup"] = "testGroup";
    header =
            static_cast<NotifyConsumerIdsChangedRequestHeader*>(NotifyConsumerIdsChangedRequestHeader::Decode(
                    ext));
    EXPECT_EQ(header->getGroup(), "testGroup");
    delete header;
}

int main(int argc, char* argv[]) {
    InitGoogleMock(&argc, argv);
    testing::GTEST_FLAG(throw_on_failure) = true;
    testing::GTEST_FLAG(filter) =
            "commandHeader.NotifyConsumerIdsChangedRequestHeader";
    int itestts = RUN_ALL_TESTS();
    return itestts;
}
