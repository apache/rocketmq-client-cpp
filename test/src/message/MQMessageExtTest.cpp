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

#include "MQMessageExt.h"
#include "MessageSysFlag.h"
#include "SocketUtil.h"
#include "TopicFilterType.h"

using testing::InitGoogleMock;
using testing::InitGoogleTest;
using testing::Return;

using rocketmq::MessageSysFlag;
using rocketmq::MQMessageClientExt;
using rocketmq::MQMessageConst;
using rocketmq::MQMessageExt;
using rocketmq::TopicFilterType;

TEST(MessageExtTest, MessageClientExt) {
  MQMessageClientExt messageClientExt;
  EXPECT_EQ(messageClientExt.getQueueOffset(), 0);
  EXPECT_EQ(messageClientExt.getCommitLogOffset(), 0);
  EXPECT_EQ(messageClientExt.getBornTimestamp(), 0);
  EXPECT_EQ(messageClientExt.getStoreTimestamp(), 0);
  EXPECT_EQ(messageClientExt.getPreparedTransactionOffset(), 0);
  EXPECT_EQ(messageClientExt.getQueueId(), 0);
  EXPECT_EQ(messageClientExt.getStoreSize(), 0);
  EXPECT_EQ(messageClientExt.getReconsumeTimes(), 3);
  EXPECT_EQ(messageClientExt.getBodyCRC(), 0);
  EXPECT_EQ(messageClientExt.getMsgId(), "");
  EXPECT_EQ(messageClientExt.getOffsetMsgId(), "");

  messageClientExt.setQueueOffset(1);
  EXPECT_EQ(messageClientExt.getQueueOffset(), 1);

  messageClientExt.setCommitLogOffset(1024);
  EXPECT_EQ(messageClientExt.getCommitLogOffset(), 1024);

  messageClientExt.setBornTimestamp(1024);
  EXPECT_EQ(messageClientExt.getBornTimestamp(), 1024);

  messageClientExt.setStoreTimestamp(2048);
  EXPECT_EQ(messageClientExt.getStoreTimestamp(), 2048);

  messageClientExt.setPreparedTransactionOffset(4096);
  EXPECT_EQ(messageClientExt.getPreparedTransactionOffset(), 4096);

  messageClientExt.setQueueId(2);
  EXPECT_EQ(messageClientExt.getQueueId(), 2);

  messageClientExt.setStoreSize(12);
  EXPECT_EQ(messageClientExt.getStoreSize(), 12);

  messageClientExt.setReconsumeTimes(48);
  EXPECT_EQ(messageClientExt.getReconsumeTimes(), 48);

  messageClientExt.setBodyCRC(32);
  EXPECT_EQ(messageClientExt.getBodyCRC(), 32);

  messageClientExt.setMsgId("MsgId");
  EXPECT_EQ(messageClientExt.getMsgId(), "");
  messageClientExt.putProperty(MQMessageConst::PROPERTY_UNIQ_CLIENT_MESSAGE_ID_KEYIDX, "MsgId");
  EXPECT_EQ(messageClientExt.getMsgId(), "MsgId");

  messageClientExt.setOffsetMsgId("offsetMsgId");
  EXPECT_EQ(messageClientExt.getOffsetMsgId(), "offsetMsgId");

  messageClientExt.setBornTimestamp(1111);
  EXPECT_EQ(messageClientExt.getBornTimestamp(), 1111);

  messageClientExt.setStoreTimestamp(2222);
  EXPECT_EQ(messageClientExt.getStoreTimestamp(), 2222);

  messageClientExt.setBornHost(rocketmq::string2SocketAddress("127.0.0.1:10091"));
  EXPECT_EQ(messageClientExt.getBornHostString(), "127.0.0.1:10091");

  messageClientExt.setStoreHost(rocketmq::string2SocketAddress("127.0.0.2:10092"));
  EXPECT_EQ(messageClientExt.getStoreHostString(), "127.0.0.2:10092");
}

TEST(MessageExtTest, MessageExt) {
  struct sockaddr* bronHost = rocketmq::copySocketAddress(nullptr, rocketmq::string2SocketAddress("127.0.0.1:10091"));
  struct sockaddr* storeHost = rocketmq::copySocketAddress(nullptr, rocketmq::string2SocketAddress("127.0.0.2:10092"));

  MQMessageExt messageExt(2, 1024, bronHost, 2048, storeHost, "msgId");
  EXPECT_EQ(messageExt.getQueueOffset(), 0);
  EXPECT_EQ(messageExt.getCommitLogOffset(), 0);
  EXPECT_EQ(messageExt.getBornTimestamp(), 1024);
  EXPECT_EQ(messageExt.getStoreTimestamp(), 2048);
  EXPECT_EQ(messageExt.getPreparedTransactionOffset(), 0);
  EXPECT_EQ(messageExt.getQueueId(), 2);
  EXPECT_EQ(messageExt.getStoreSize(), 0);
  EXPECT_EQ(messageExt.getReconsumeTimes(), 3);
  EXPECT_EQ(messageExt.getBodyCRC(), 0);
  EXPECT_EQ(messageExt.getMsgId(), "msgId");
  EXPECT_EQ(messageExt.getBornHostString(), "127.0.0.1:10091");
  EXPECT_EQ(messageExt.getStoreHostString(), "127.0.0.2:10092");

  free(bronHost);
  free(storeHost);
}

TEST(MessageExtTest, ParseTopicFilterType) {
  EXPECT_EQ(MQMessageExt::parseTopicFilterType(MessageSysFlag::MultiTagsFlag), TopicFilterType::MULTI_TAG);
  EXPECT_EQ(MQMessageExt::parseTopicFilterType(0), TopicFilterType::SINGLE_TAG);
}

int main(int argc, char* argv[]) {
  InitGoogleMock(&argc, argv);
  testing::GTEST_FLAG(throw_on_failure) = true;
  testing::GTEST_FLAG(filter) = "MessageExtTest.*";
  return RUN_ALL_TESTS();
}
