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
#include "c/CCommon.h"
#include "c/CMessageExt.h"

using testing::InitGoogleMock;
using testing::InitGoogleTest;
using testing::Return;

using rocketmq::MQMessageExt;

TEST(CMessageExtTest, CheckProperties) {
  MQMessageExt* mqMessageExt = new MQMessageExt();
  CMessageExt* messageExt = (CMessageExt*)mqMessageExt;

  mqMessageExt->set_topic("testTopic");
  EXPECT_EQ(GetMessageTopic(messageExt), mqMessageExt->topic());

  mqMessageExt->set_tags("testTags");
  EXPECT_EQ(GetMessageTags(messageExt), mqMessageExt->tags());

  mqMessageExt->set_keys("testKeys");
  EXPECT_EQ(GetMessageKeys(messageExt), mqMessageExt->keys());

  mqMessageExt->set_body("testBody");
  EXPECT_EQ(GetMessageBody(messageExt), mqMessageExt->body());

  mqMessageExt->putProperty("testProperty", "testValue");
  EXPECT_EQ(GetMessageProperty(messageExt, "testProperty"), mqMessageExt->getProperty("testProperty"));

  mqMessageExt->set_msg_id("msgId123456");
  EXPECT_EQ(GetMessageId(messageExt), mqMessageExt->msg_id());

  mqMessageExt->set_delay_time_level(1);
  EXPECT_EQ(GetMessageDelayTimeLevel(messageExt), mqMessageExt->delay_time_level());

  mqMessageExt->set_queue_id(4);
  EXPECT_EQ(GetMessageQueueId(messageExt), mqMessageExt->queue_id());

  mqMessageExt->set_reconsume_times(1234567);
  EXPECT_EQ(GetMessageReconsumeTimes(messageExt), mqMessageExt->reconsume_times());

  mqMessageExt->set_store_size(127);
  EXPECT_EQ(GetMessageStoreSize(messageExt), mqMessageExt->store_size());

  mqMessageExt->set_born_timestamp(9876543);
  EXPECT_EQ(GetMessageBornTimestamp(messageExt), mqMessageExt->born_timestamp());

  mqMessageExt->set_store_timestamp(123123);
  EXPECT_EQ(GetMessageStoreTimestamp(messageExt), mqMessageExt->store_timestamp());

  mqMessageExt->set_queue_offset(1024);
  EXPECT_EQ(GetMessageQueueOffset(messageExt), mqMessageExt->queue_offset());

  mqMessageExt->set_commit_log_offset(2048);
  EXPECT_EQ(GetMessageCommitLogOffset(messageExt), mqMessageExt->commit_log_offset());

  mqMessageExt->set_prepared_transaction_offset(4096);
  EXPECT_EQ(GetMessagePreparedTransactionOffset(messageExt), mqMessageExt->prepared_transaction_offset());

  delete mqMessageExt;
}

TEST(CMessageExtTest, CheckNull) {
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
  testing::GTEST_FLAG(throw_on_failure) = true;
  testing::GTEST_FLAG(filter) = "CMessageExtTest.*";
  return RUN_ALL_TESTS();
}
