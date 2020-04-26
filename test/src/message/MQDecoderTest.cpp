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
#include <stdio.h>

#include <string>
#include <vector>

#include "MQDecoder.h"
#include "MQMessage.h"
#include "MQMessageConst.h"
#include "MQMessageExt.h"
#include "MQMessageId.h"
#include "MemoryInputStream.h"
#include "MemoryOutputStream.h"
#include "MessageSysFlag.h"
#include "RemotingCommand.h"
#include "UtilAll.h"
#include "protocol/header/CommandHeader.h"

using testing::InitGoogleMock;
using testing::InitGoogleTest;
using testing::Return;

using rocketmq::MemoryBlock;
using rocketmq::MemoryInputStream;
using rocketmq::MemoryOutputStream;
using rocketmq::MessageSysFlag;
using rocketmq::MQDecoder;
using rocketmq::MQMessage;
using rocketmq::MQMessageConst;
using rocketmq::MQMessageExt;
using rocketmq::MQMessageId;
using rocketmq::RemotingCommand;
using rocketmq::SendMessageRequestHeader;
using rocketmq::UtilAll;

// TODO
TEST(MessageDecoderTest, MessageId) {
  std::string strMsgId = MQDecoder::createMessageId(rocketmq::string2SocketAddress("127.0.0.1:10091"), 1024LL);
  EXPECT_EQ(strMsgId, "0100007F0000276B0000000000000400");

  MQMessageId msgId = MQDecoder::decodeMessageId(strMsgId);
  EXPECT_EQ(msgId.getOffset(), 1024);
}

TEST(MessageDecoderTest, Decode) {
  MemoryOutputStream* memoryOut = new MemoryOutputStream(1024);
  MQMessageExt msgExt;

  // 1 TOTALSIZE  4=0+4
  memoryOut->writeIntBigEndian(111);
  msgExt.setStoreSize(111);

  // 2 MAGICCODE sizeof(int)  8=4+4
  memoryOut->writeIntBigEndian(14);

  // 3 BODYCRC  12=8+4
  memoryOut->writeIntBigEndian(24);
  msgExt.setBodyCRC(24);

  // 4 QUEUEID  16=12+4
  memoryOut->writeIntBigEndian(4);
  msgExt.setQueueId(4);

  // 5 FLAG  20=16+4
  memoryOut->writeIntBigEndian(4);
  msgExt.setFlag(4);

  // 6 QUEUEOFFSET  28=20+8
  memoryOut->writeInt64BigEndian(1024LL);
  msgExt.setQueueOffset(1024LL);

  // 7 PHYSICALOFFSET  36=28+8
  memoryOut->writeInt64BigEndian(2048LL);
  msgExt.setCommitLogOffset(2048LL);

  // 8 SYSFLAG  40=36+4
  memoryOut->writeIntBigEndian(0);
  msgExt.setSysFlag(0);

  // 9 BORNTIMESTAMP  48=40+8
  memoryOut->writeInt64BigEndian(4096LL);
  msgExt.setBornTimestamp(4096LL);

  // 10 BORNHOST  56=48+4+4
  memoryOut->writeIntBigEndian(ntohl(inet_addr("127.0.0.1")));
  memoryOut->writeIntBigEndian(10091);
  msgExt.setBornHost(rocketmq::string2SocketAddress("127.0.0.1:10091"));

  // 11 STORETIMESTAMP  64=56+8
  memoryOut->writeInt64BigEndian(4096LL);
  msgExt.setStoreTimestamp(4096LL);

  // 12 STOREHOST  72=64+4+4
  memoryOut->writeIntBigEndian(ntohl(inet_addr("127.0.0.2")));
  memoryOut->writeIntBigEndian(10092);
  msgExt.setStoreHost(rocketmq::string2SocketAddress("127.0.0.2:10092"));

  // 13 RECONSUMETIMES 76=72+4
  memoryOut->writeIntBigEndian(18);
  msgExt.setReconsumeTimes(18);

  // 14 Prepared Transaction Offset  84=76+8
  memoryOut->writeInt64BigEndian(12LL);
  msgExt.setPreparedTransactionOffset(12LL);

  // 15 BODY  98=84+4+10
  std::string body = "1234567890";
  memoryOut->writeIntBigEndian(body.size());
  memoryOut->write(body.data(), body.size());
  msgExt.setBody(body);

  // 16 TOPIC  109=98+1+10
  std::string topic = "topic_1234";
  memoryOut->writeByte(topic.size());
  memoryOut->write(topic.data(), topic.size());
  msgExt.setTopic(topic);

  // 17 PROPERTIES 111=109+2
  memoryOut->writeShortBigEndian(0);

  msgExt.setMsgId(MQDecoder::createMessageId(msgExt.getStoreHost(), (int64_t)msgExt.getCommitLogOffset()));

  auto block = memoryOut->getMemoryBlock();
  auto msgs = MQDecoder::decodes(*static_cast<MemoryBlock*>(&block));
  EXPECT_EQ(msgs.size(), 1);

  std::cout << msgs[0]->toString() << std::endl;
  std::cout << msgExt.toString() << std::endl;
  EXPECT_EQ(msgs[0]->toString(), msgExt.toString());

  msgs = MQDecoder::decodes(*static_cast<MemoryBlock*>(&block), false);
  EXPECT_EQ(msgs[0]->getBody().size(), 0);

  //===============================================================

  // 8 SYSFLAG  40=36+4
  memoryOut->setPosition(36);
  memoryOut->writeIntBigEndian(0 | MessageSysFlag::CompressedFlag);
  msgExt.setSysFlag(0 | MessageSysFlag::CompressedFlag);

  // 15 Body 84
  std::string compressedBody;
  UtilAll::deflate(body, compressedBody, 5);
  memoryOut->setPosition(84);
  memoryOut->writeIntBigEndian(compressedBody.size());
  memoryOut->write(compressedBody.data(), compressedBody.size());
  msgExt.setBody(compressedBody);

  // 16 TOPIC
  memoryOut->writeByte(topic.size());
  memoryOut->write(topic.data(), topic.size());
  msgExt.setTopic(topic);

  // 17 PROPERTIES
  std::map<std::string, std::string> properties;
  properties["RocketMQ"] = "cpp-client";
  properties[MQMessageConst::PROPERTY_UNIQ_CLIENT_MESSAGE_ID_KEYIDX] = "123456";
  std::string props = MQDecoder::messageProperties2String(properties);
  memoryOut->writeShortBigEndian(props.size());
  memoryOut->write(props.data(), props.size());
  msgExt.setProperties(properties);
  msgExt.setMsgId("123456");

  memoryOut->setPosition(0);
  memoryOut->writeIntBigEndian(memoryOut->getDataSize());
  msgExt.setStoreSize(memoryOut->getDataSize());

  block = memoryOut->getMemoryBlock();
  msgs = MQDecoder::decodes(*static_cast<MemoryBlock*>(&block));
  EXPECT_EQ(msgs[0]->toString(), msgExt.toString());
}

TEST(MessageDecoderTest, MessagePropertiesAndToString) {
  std::map<std::string, std::string> properties;
  properties["RocketMQ"] = "cpp-client";
  std::string props = MQDecoder::messageProperties2String(properties);
  EXPECT_EQ(props, "RocketMQ\001cpp-client\002");

  auto properties2 = MQDecoder::string2messageProperties(props);
  EXPECT_EQ(properties, properties2);
}

int main(int argc, char* argv[]) {
  InitGoogleMock(&argc, argv);
  testing::GTEST_FLAG(throw_on_failure) = true;
  testing::GTEST_FLAG(filter) = "MessageDecoderTest.*";
  return RUN_ALL_TESTS();
}
