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
#include "MessageExtImpl.h"
#include "MessageSysFlag.h"
#include "SocketUtil.h"
#include "TopicFilterType.h"

using testing::InitGoogleMock;
using testing::InitGoogleTest;
using testing::Return;

using rocketmq::MessageClientExtImpl;
using rocketmq::MessageExtImpl;
using rocketmq::MessageSysFlag;
using rocketmq::MQMessageConst;
using rocketmq::MQMessageExt;
using rocketmq::TopicFilterType;

TEST(MessageExtTest, MessageClientExtImpl) {
  MessageClientExtImpl messageClientExt;
  EXPECT_EQ(messageClientExt.queue_offset(), 0);
  EXPECT_EQ(messageClientExt.commit_log_offset(), 0);
  EXPECT_EQ(messageClientExt.born_timestamp(), 0);
  EXPECT_EQ(messageClientExt.store_timestamp(), 0);
  EXPECT_EQ(messageClientExt.prepared_transaction_offset(), 0);
  EXPECT_EQ(messageClientExt.queue_id(), 0);
  EXPECT_EQ(messageClientExt.store_size(), 0);
  EXPECT_EQ(messageClientExt.reconsume_times(), 3);
  EXPECT_EQ(messageClientExt.body_crc(), 0);
  EXPECT_EQ(messageClientExt.msg_id(), "");
  EXPECT_EQ(messageClientExt.offset_msg_id(), "");

  messageClientExt.set_queue_offset(1);
  EXPECT_EQ(messageClientExt.queue_offset(), 1);

  messageClientExt.set_commit_log_offset(1024);
  EXPECT_EQ(messageClientExt.commit_log_offset(), 1024);

  messageClientExt.set_born_timestamp(1024);
  EXPECT_EQ(messageClientExt.born_timestamp(), 1024);

  messageClientExt.set_store_timestamp(2048);
  EXPECT_EQ(messageClientExt.store_timestamp(), 2048);

  messageClientExt.set_prepared_transaction_offset(4096);
  EXPECT_EQ(messageClientExt.prepared_transaction_offset(), 4096);

  messageClientExt.set_queue_id(2);
  EXPECT_EQ(messageClientExt.queue_id(), 2);

  messageClientExt.set_store_size(12);
  EXPECT_EQ(messageClientExt.store_size(), 12);

  messageClientExt.set_reconsume_times(48);
  EXPECT_EQ(messageClientExt.reconsume_times(), 48);

  messageClientExt.set_body_crc(32);
  EXPECT_EQ(messageClientExt.body_crc(), 32);

  messageClientExt.set_msg_id("MsgId");
  EXPECT_EQ(messageClientExt.msg_id(), "");
  messageClientExt.putProperty(MQMessageConst::PROPERTY_UNIQ_CLIENT_MESSAGE_ID_KEYIDX, "MsgId");
  EXPECT_EQ(messageClientExt.msg_id(), "MsgId");

  messageClientExt.set_offset_msg_id("offsetMsgId");
  EXPECT_EQ(messageClientExt.offset_msg_id(), "offsetMsgId");

  messageClientExt.set_born_timestamp(1111);
  EXPECT_EQ(messageClientExt.born_timestamp(), 1111);

  messageClientExt.set_store_timestamp(2222);
  EXPECT_EQ(messageClientExt.store_timestamp(), 2222);

  messageClientExt.set_born_host(rocketmq::StringToSockaddr("127.0.0.1:10091"));
  EXPECT_EQ(messageClientExt.born_host_string(), "127.0.0.1:10091");

  messageClientExt.set_store_host(rocketmq::StringToSockaddr("127.0.0.2:10092"));
  EXPECT_EQ(messageClientExt.store_host_string(), "127.0.0.2:10092");
}

TEST(MessageExtTest, MessageExt) {
  auto bronHost = rocketmq::SockaddrToStorage(rocketmq::StringToSockaddr("127.0.0.1:10091"));
  auto storeHost = rocketmq::SockaddrToStorage(rocketmq::StringToSockaddr("127.0.0.2:10092"));

  MQMessageExt messageExt(2, 1024, reinterpret_cast<sockaddr*>(bronHost.get()), 2048,
                          reinterpret_cast<sockaddr*>(storeHost.get()), "msgId");
  EXPECT_EQ(messageExt.queue_offset(), 0);
  EXPECT_EQ(messageExt.commit_log_offset(), 0);
  EXPECT_EQ(messageExt.born_timestamp(), 1024);
  EXPECT_EQ(messageExt.store_timestamp(), 2048);
  EXPECT_EQ(messageExt.prepared_transaction_offset(), 0);
  EXPECT_EQ(messageExt.queue_id(), 2);
  EXPECT_EQ(messageExt.store_size(), 0);
  EXPECT_EQ(messageExt.reconsume_times(), 3);
  EXPECT_EQ(messageExt.body_crc(), 0);
  EXPECT_EQ(messageExt.msg_id(), "msgId");
  EXPECT_EQ(messageExt.born_host_string(), "127.0.0.1:10091");
  EXPECT_EQ(messageExt.store_host_string(), "127.0.0.2:10092");
}

TEST(MessageExtTest, ParseTopicFilterType) {
  EXPECT_EQ(MessageExtImpl::parseTopicFilterType(MessageSysFlag::MULTI_TAGS_FLAG), TopicFilterType::MULTI_TAG);
  EXPECT_EQ(MessageExtImpl::parseTopicFilterType(0), TopicFilterType::SINGLE_TAG);
}

int main(int argc, char* argv[]) {
  InitGoogleMock(&argc, argv);
  testing::GTEST_FLAG(throw_on_failure) = true;
  testing::GTEST_FLAG(filter) = "MessageExtTest.*";
  return RUN_ALL_TESTS();
}
