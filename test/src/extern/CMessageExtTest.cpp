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
#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "CCommon.h"
#include "CMessageExt.h"
#include "MQMessageExt.h"

using ::testing::InitGoogleMock;
using ::testing::InitGoogleTest;
using testing::Return;

using rocketmq::MQMessageExt;

TEST(cmessageExt, info) {
  MQMessageExt* mqMessageExt = new MQMessageExt();
  CMessageExt* messageExt = (CMessageExt*)mqMessageExt;

  mqMessageExt->setTopic("testTopic");
  EXPECT_EQ(GetMessageTopic(messageExt), mqMessageExt->getTopic());

  mqMessageExt->setTags("testTags");
  EXPECT_EQ(GetMessageTags(messageExt), mqMessageExt->getTags());

  mqMessageExt->setKeys("testKeys");
  EXPECT_EQ(GetMessageKeys(messageExt), mqMessageExt->getKeys());

  std::string body("testBody");
  body.append(3, '\0');
  mqMessageExt->setBody(body.c_str(), body.length());
  std::string retrieved_body(GetMessageBody(messageExt), GetMessageBodyLength(messageExt));
  EXPECT_TRUE(body == retrieved_body);

  mqMessageExt->setProperty("testKey", "testValues");
  EXPECT_EQ(GetMessageProperty(messageExt, "testKey"), mqMessageExt->getProperty("testKey"));

  mqMessageExt->setMsgId("msgId123456");
  EXPECT_EQ(GetMessageId(messageExt), mqMessageExt->getMsgId());

  mqMessageExt->setDelayTimeLevel(1);
  EXPECT_EQ(GetMessageDelayTimeLevel(messageExt), mqMessageExt->getDelayTimeLevel());

  mqMessageExt->setQueueId(4);
  EXPECT_EQ(GetMessageQueueId(messageExt), mqMessageExt->getQueueId());

  mqMessageExt->setReconsumeTimes(1234567);
  EXPECT_EQ(GetMessageReconsumeTimes(messageExt), mqMessageExt->getReconsumeTimes());

  mqMessageExt->setStoreSize(127);
  EXPECT_EQ(GetMessageStoreSize(messageExt), mqMessageExt->getStoreSize());

  mqMessageExt->setBornTimestamp(9876543);
  EXPECT_EQ(GetMessageBornTimestamp(messageExt), mqMessageExt->getBornTimestamp());

  mqMessageExt->setStoreTimestamp(123123);
  EXPECT_EQ(GetMessageStoreTimestamp(messageExt), mqMessageExt->getStoreTimestamp());

  mqMessageExt->setQueueOffset(1024);
  EXPECT_EQ(GetMessageQueueOffset(messageExt), mqMessageExt->getQueueOffset());

  mqMessageExt->setCommitLogOffset(2048);
  EXPECT_EQ(GetMessageCommitLogOffset(messageExt), mqMessageExt->getCommitLogOffset());

  mqMessageExt->setPreparedTransactionOffset(4096);
  EXPECT_EQ(GetMessagePreparedTransactionOffset(messageExt), mqMessageExt->getPreparedTransactionOffset());

  delete mqMessageExt;
}

TEST(cmessageExt, null) {
  EXPECT_TRUE(GetMessageTopic(NULL) == NULL);
  EXPECT_TRUE(GetMessageTags(NULL) == NULL);
  EXPECT_TRUE(GetMessageKeys(NULL) == NULL);
  EXPECT_TRUE(GetMessageBody(NULL) == NULL);
  EXPECT_TRUE(GetMessageProperty(NULL, NULL) == NULL);
  EXPECT_TRUE(GetMessageId(NULL) == NULL);
  EXPECT_EQ(GetMessageDelayTimeLevel(NULL), NULL_POINTER);
  EXPECT_EQ(GetMessageQueueId(NULL), NULL_POINTER);
  EXPECT_EQ(GetMessageReconsumeTimes(NULL), NULL_POINTER);
  EXPECT_EQ(GetMessageStoreSize(NULL), NULL_POINTER);
  EXPECT_EQ(GetMessageBornTimestamp(NULL), NULL_POINTER);
  EXPECT_EQ(GetMessageStoreTimestamp(NULL), NULL_POINTER);
  EXPECT_EQ(GetMessageQueueOffset(NULL), NULL_POINTER);
  EXPECT_EQ(GetMessageCommitLogOffset(NULL), NULL_POINTER);
  EXPECT_EQ(GetMessagePreparedTransactionOffset(NULL), NULL_POINTER);
}

int main(int argc, char* argv[]) {
  InitGoogleMock(&argc, argv);
  testing::GTEST_FLAG(filter) = "cmessageExt.*";
  int itestts = RUN_ALL_TESTS();
  return itestts;
}
