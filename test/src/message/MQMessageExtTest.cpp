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

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "MQMessageExt.h"
#include "MessageSysFlag.h"
#include "SocketUtil.h"
#include "TopicFilterType.h"

using ::testing::InitGoogleMock;
using ::testing::InitGoogleTest;
using testing::Return;

using rocketmq::MessageSysFlag;
using rocketmq::MQMessageExt;
using rocketmq::TopicFilterType;

TEST(messageExt, init) {
  MQMessageExt messageExt;
  EXPECT_EQ(messageExt.getQueueOffset(), 0);
  EXPECT_EQ(messageExt.getCommitLogOffset(), 0);
  EXPECT_EQ(messageExt.getBornTimestamp(), 0);
  EXPECT_EQ(messageExt.getStoreTimestamp(), 0);
  EXPECT_EQ(messageExt.getPreparedTransactionOffset(), 0);
  EXPECT_EQ(messageExt.getQueueId(), 0);
  EXPECT_EQ(messageExt.getStoreSize(), 0);
  EXPECT_EQ(messageExt.getReconsumeTimes(), 3);
  EXPECT_EQ(messageExt.getBodyCRC(), 0);
  EXPECT_EQ(messageExt.getMsgId(), "");
  EXPECT_EQ(messageExt.getOffsetMsgId(), "");

  messageExt.setQueueOffset(1);
  EXPECT_EQ(messageExt.getQueueOffset(), 1);

  messageExt.setCommitLogOffset(1024);
  EXPECT_EQ(messageExt.getCommitLogOffset(), 1024);

  messageExt.setBornTimestamp(1024);
  EXPECT_EQ(messageExt.getBornTimestamp(), 1024);

  messageExt.setStoreTimestamp(2048);
  EXPECT_EQ(messageExt.getStoreTimestamp(), 2048);

  messageExt.setPreparedTransactionOffset(4096);
  EXPECT_EQ(messageExt.getPreparedTransactionOffset(), 4096);

  messageExt.setQueueId(2);
  EXPECT_EQ(messageExt.getQueueId(), 2);

  messageExt.setStoreSize(12);
  EXPECT_EQ(messageExt.getStoreSize(), 12);

  messageExt.setReconsumeTimes(48);
  EXPECT_EQ(messageExt.getReconsumeTimes(), 48);

  messageExt.setBodyCRC(32);
  EXPECT_EQ(messageExt.getBodyCRC(), 32);

  messageExt.setMsgId("MsgId");
  EXPECT_EQ(messageExt.getMsgId(), "MsgId");

  messageExt.setOffsetMsgId("offsetMsgId");
  EXPECT_EQ(messageExt.getOffsetMsgId(), "offsetMsgId");

  messageExt.setBornTimestamp(1111);
  EXPECT_EQ(messageExt.getBornTimestamp(), 1111);

  messageExt.setStoreTimestamp(2222);
  EXPECT_EQ(messageExt.getStoreTimestamp(), 2222);

  struct sockaddr_in sa;
  sa.sin_family = AF_INET;
  sa.sin_port = htons(10091);
  sa.sin_addr.s_addr = inet_addr("127.0.0.1");

  sockaddr bornHost;
  memcpy(&bornHost, &sa, sizeof(sockaddr));

  messageExt.setBornHost(bornHost);
  EXPECT_EQ(messageExt.getBornHostNameString(), rocketmq::getHostName(bornHost));
  EXPECT_EQ(messageExt.getBornHostString(), rocketmq::socketAddress2String(bornHost));

  struct sockaddr_in storeSa;
  storeSa.sin_family = AF_INET;
  storeSa.sin_port = htons(10092);
  storeSa.sin_addr.s_addr = inet_addr("127.0.0.2");

  sockaddr storeHost;
  memcpy(&storeHost, &storeSa, sizeof(sockaddr));
  messageExt.setStoreHost(storeHost);
  EXPECT_EQ(messageExt.getStoreHostString(), rocketmq::socketAddress2String(storeHost));

  MQMessageExt twoMessageExt(2, 1024, bornHost, 2048, storeHost, "msgId");
  EXPECT_EQ(twoMessageExt.getQueueOffset(), 0);
  EXPECT_EQ(twoMessageExt.getCommitLogOffset(), 0);
  EXPECT_EQ(twoMessageExt.getBornTimestamp(), 1024);
  EXPECT_EQ(twoMessageExt.getStoreTimestamp(), 2048);
  EXPECT_EQ(twoMessageExt.getPreparedTransactionOffset(), 0);
  EXPECT_EQ(twoMessageExt.getQueueId(), 2);
  EXPECT_EQ(twoMessageExt.getStoreSize(), 0);
  EXPECT_EQ(twoMessageExt.getReconsumeTimes(), 3);
  EXPECT_EQ(twoMessageExt.getBodyCRC(), 0);
  EXPECT_EQ(twoMessageExt.getMsgId(), "msgId");
  EXPECT_EQ(twoMessageExt.getOffsetMsgId(), "");

  EXPECT_EQ(twoMessageExt.getBornHostNameString(), rocketmq::getHostName(bornHost));
  EXPECT_EQ(twoMessageExt.getBornHostString(), rocketmq::socketAddress2String(bornHost));

  EXPECT_EQ(twoMessageExt.getStoreHostString(), rocketmq::socketAddress2String(storeHost));

  EXPECT_EQ(MQMessageExt::parseTopicFilterType(MessageSysFlag::MultiTagsFlag), TopicFilterType::MULTI_TAG);

  EXPECT_EQ(MQMessageExt::parseTopicFilterType(0), TopicFilterType::SINGLE_TAG);
}

int main(int argc, char* argv[]) {
  InitGoogleMock(&argc, argv);

  testing::GTEST_FLAG(filter) = "messageExt.*";
  int itestts = RUN_ALL_TESTS();
  return itestts;
}
