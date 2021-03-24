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
#include "MessageDecoder.h"

#include <string>
#include <vector>

#include <arpa/inet.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "ByteArray.h"
#include "ByteBuffer.hpp"
#include "MQMessage.h"
#include "MQMessageConst.h"
#include "MQMessageExt.h"
#include "MessageId.h"
#include "MessageSysFlag.h"
#include "RemotingCommand.h"
#include "UtilAll.h"
#include "protocol/header/CommandHeader.h"

using testing::InitGoogleMock;
using testing::InitGoogleTest;
using testing::Return;

using rocketmq::ByteArray;
using rocketmq::ByteBuffer;
using rocketmq::MessageDecoder;
using rocketmq::MessageId;
using rocketmq::MessageSysFlag;
using rocketmq::MQMessage;
using rocketmq::MQMessageConst;
using rocketmq::MQMessageExt;
using rocketmq::RemotingCommand;
using rocketmq::SendMessageRequestHeader;
using rocketmq::stoba;
using rocketmq::UtilAll;

// TODO
TEST(MessageDecoderTest, MessageId) {
  std::string strMsgId = MessageDecoder::createMessageId(rocketmq::StringToSockaddr("127.0.0.1:10091"), 1024LL);
  EXPECT_EQ(strMsgId, "7F0000010000276B0000000000000400");

  MessageId msgId = MessageDecoder::decodeMessageId(strMsgId);
  EXPECT_EQ(msgId.getOffset(), 1024LL);

  std::string strMsgId2 = MessageDecoder::createMessageId(rocketmq::StringToSockaddr("/172.16.2.114:0"), 123456LL);
  EXPECT_EQ(strMsgId2, "AC10027200000000000000000001E240");

  MessageId msgId2 = MessageDecoder::decodeMessageId(strMsgId2);
  EXPECT_EQ(msgId2.getOffset(), 123456LL);
}

TEST(MessageDecoderTest, Decode) {
  std::unique_ptr<ByteBuffer> byteBuffer(ByteBuffer::allocate(1024));
  MQMessageExt msgExt;

  // 1 TOTALSIZE  4=0+4
  byteBuffer->putInt(111);
  msgExt.set_store_size(111);

  // 2 MAGICCODE sizeof(int)  8=4+4
  byteBuffer->putInt(14);

  // 3 BODYCRC  12=8+4
  byteBuffer->putInt(24);
  msgExt.set_body_crc(24);

  // 4 QUEUEID  16=12+4
  byteBuffer->putInt(4);
  msgExt.set_queue_id(4);

  // 5 FLAG  20=16+4
  byteBuffer->putInt(4);
  msgExt.set_flag(4);

  // 6 QUEUEOFFSET  28=20+8
  byteBuffer->putLong(1024LL);
  msgExt.set_queue_offset(1024LL);

  // 7 PHYSICALOFFSET  36=28+8
  byteBuffer->putLong(2048LL);
  msgExt.set_commit_log_offset(2048LL);

  // 8 SYSFLAG  40=36+4
  byteBuffer->putInt(0);
  msgExt.set_sys_flag(0);

  // 9 BORNTIMESTAMP  48=40+8
  byteBuffer->putLong(4096LL);
  msgExt.set_born_timestamp(4096LL);

  // 10 BORNHOST  56=48+4+4
  byteBuffer->putInt(ntohl(inet_addr("127.0.0.1")));
  byteBuffer->putInt(10091);
  msgExt.set_born_host(rocketmq::StringToSockaddr("127.0.0.1:10091"));

  // 11 STORETIMESTAMP  64=56+8
  byteBuffer->putLong(4096LL);
  msgExt.set_store_timestamp(4096LL);

  // 12 STOREHOST  72=64+4+4
  byteBuffer->putInt(ntohl(inet_addr("127.0.0.2")));
  byteBuffer->putInt(10092);
  msgExt.set_store_host(rocketmq::StringToSockaddr("127.0.0.2:10092"));

  // 13 RECONSUMETIMES 76=72+4
  byteBuffer->putInt(18);
  msgExt.set_reconsume_times(18);

  // 14 Prepared Transaction Offset  84=76+8
  byteBuffer->putLong(12LL);
  msgExt.set_prepared_transaction_offset(12LL);

  // 15 BODY  98=84+4+10
  std::string body("1234567890");
  byteBuffer->putInt(body.size());
  byteBuffer->put(*stoba(body));
  msgExt.set_body(body);

  // 16 TOPIC  109=98+1+10
  std::string topic = "topic_1234";
  byteBuffer->put((int8_t)topic.size());
  byteBuffer->put(*stoba(topic));
  msgExt.set_topic(topic);

  // 17 PROPERTIES 111=109+2
  byteBuffer->putShort(0);

  msgExt.set_msg_id(MessageDecoder::createMessageId(msgExt.store_host(), (int64_t)msgExt.commit_log_offset()));

  byteBuffer->flip();
  auto msgs = MessageDecoder::decodes(*byteBuffer);
  EXPECT_EQ(msgs.size(), 1);

  std::cout << msgs[0]->toString() << std::endl;
  std::cout << msgExt.toString() << std::endl;
  EXPECT_EQ(msgs[0]->toString(), msgExt.toString());

  byteBuffer->rewind();
  msgs = MessageDecoder::decodes(*byteBuffer, false);
  EXPECT_EQ(msgs[0]->body(), "");

  //===============================================================

  byteBuffer->clear();

  // 8 SYSFLAG  40=36+4
  byteBuffer->position(36);
  byteBuffer->putInt(0 | MessageSysFlag::COMPRESSED_FLAG);
  msgExt.set_sys_flag(0 | MessageSysFlag::COMPRESSED_FLAG);

  // 15 Body 84
  std::string compressedBody;
  UtilAll::deflate(body, compressedBody, 5);
  byteBuffer->position(84);
  byteBuffer->putInt(compressedBody.size());
  byteBuffer->put(*stoba(compressedBody));
  msgExt.set_body(compressedBody);

  // 16 TOPIC
  byteBuffer->put((int8_t)topic.size());
  byteBuffer->put(*stoba(topic));
  msgExt.set_topic(topic);

  // 17 PROPERTIES
  std::map<std::string, std::string> properties;
  properties["RocketMQ"] = "cpp-client";
  properties[MQMessageConst::PROPERTY_UNIQ_CLIENT_MESSAGE_ID_KEYIDX] = "123456";
  std::string props = MessageDecoder::messageProperties2String(properties);
  byteBuffer->putShort(props.size());
  byteBuffer->put(*stoba(props));
  msgExt.set_properties(properties);
  msgExt.set_msg_id("123456");

  byteBuffer->flip();

  byteBuffer->putInt(byteBuffer->limit());
  msgExt.set_store_size(byteBuffer->limit());

  byteBuffer->rewind();
  msgs = MessageDecoder::decodes(*byteBuffer);
  EXPECT_EQ(msgs[0]->toString(), msgExt.toString());
}

TEST(MessageDecoderTest, MessagePropertiesAndToString) {
  std::map<std::string, std::string> properties;
  properties["RocketMQ"] = "cpp-client";
  std::string props = MessageDecoder::messageProperties2String(properties);
  EXPECT_EQ(props, "RocketMQ\001cpp-client\002");

  auto properties2 = MessageDecoder::string2messageProperties(props);
  EXPECT_EQ(properties, properties2);
}

int main(int argc, char* argv[]) {
  InitGoogleMock(&argc, argv);
  testing::GTEST_FLAG(throw_on_failure) = true;
  testing::GTEST_FLAG(filter) = "MessageDecoderTest.*";
  return RUN_ALL_TESTS();
}
