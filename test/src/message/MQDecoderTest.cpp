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

#include <stdio.h>
#include <string>
#include <vector>
#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "MemoryInputStream.h"
#include "MemoryOutputStream.h"

#include "CommandHeader.h"
#include "MQDecoder.h"
#include "MQMessage.h"
#include "MQMessageExt.h"
#include "MQMessageId.h"
#include "MessageSysFlag.h"
#include "RemotingCommand.h"
#include "UtilAll.h"

using namespace std;

using ::testing::InitGoogleMock;
using ::testing::InitGoogleTest;
using testing::Return;

using rocketmq::MemoryBlock;
using rocketmq::MemoryInputStream;
using rocketmq::MemoryOutputStream;
using rocketmq::MessageSysFlag;
using rocketmq::MQDecoder;
using rocketmq::MQMessage;
using rocketmq::MQMessageExt;
using rocketmq::MQMessageId;
using rocketmq::RemotingCommand;
using rocketmq::SendMessageRequestHeader;
using rocketmq::UtilAll;

// TODO
TEST(decoder, messageId) {
  int host;
  int port;
  int64 offset = 1234567890;
  string msgIdStr =
      MQDecoder::createMessageId(rocketmq::IPPort2socketAddress(ntohl(inet_addr("127.0.0.1")), 10091), offset);
  MQMessageId msgId = MQDecoder::decodeMessageId(msgIdStr);

  EXPECT_EQ(msgId.getOffset(), offset);

  rocketmq::socketAddress2IPPort(msgId.getAddress(), host, port);
  EXPECT_EQ(host, ntohl(inet_addr("127.0.0.1")));
  EXPECT_EQ(port, 10091);
}

TEST(decoder, decoder) {
  MQMessageExt mext;
  MemoryOutputStream* memoryOut = new MemoryOutputStream(1024);

  // 1 TOTALSIZE 4
  memoryOut->writeIntBigEndian(107);
  mext.setStoreSize(107);

  // 2 MAGICCODE sizeof(int)  8=4+4
  memoryOut->writeIntBigEndian(14);

  // 3 BODYCRC  12=8+4
  memoryOut->writeIntBigEndian(24);
  mext.setBodyCRC(24);
  // 4 QUEUEID 16=12+4
  memoryOut->writeIntBigEndian(4);
  mext.setQueueId(4);
  // 5 FLAG    20=16+4
  memoryOut->writeIntBigEndian(4);
  mext.setFlag(4);
  // 6 QUEUEOFFSET  28 = 20+8
  memoryOut->writeInt64BigEndian((int64)1024);
  mext.setQueueOffset(1024);
  // 7 PHYSICALOFFSET 36=28+8
  memoryOut->writeInt64BigEndian((int64)2048);
  mext.setCommitLogOffset(2048);
  // 8 SYSFLAG  40=36+4
  memoryOut->writeIntBigEndian(0);
  mext.setSysFlag(0);
  // 9 BORNTIMESTAMP 48 = 40+8
  memoryOut->writeInt64BigEndian((int64)4096);
  mext.setBornTimestamp(4096);
  // 10 BORNHOST 56= 48+8
  memoryOut->writeIntBigEndian(ntohl(inet_addr("127.0.0.1")));
  memoryOut->writeIntBigEndian(10091);
  mext.setBornHost(rocketmq::IPPort2socketAddress(ntohl(inet_addr("127.0.0.1")), 10091));
  // 11 STORETIMESTAMP 64 =56+8
  memoryOut->writeInt64BigEndian((int64)4096);
  mext.setStoreTimestamp(4096);
  // 12 STOREHOST 72 = 64+8
  memoryOut->writeIntBigEndian(ntohl(inet_addr("127.0.0.2")));
  memoryOut->writeIntBigEndian(10092);
  mext.setStoreHost(rocketmq::IPPort2socketAddress(ntohl(inet_addr("127.0.0.2")), 10092));
  // 13 RECONSUMETIMES 76 = 72+4
  mext.setReconsumeTimes(111111);
  memoryOut->writeIntBigEndian(mext.getReconsumeTimes());
  // 14 Prepared Transaction Offset 84 = 76+8
  memoryOut->writeInt64BigEndian((int64)12);
  mext.setPreparedTransactionOffset(12);
  // 15 BODY 88 = 84+4   10
  string* body = new string("1234567890");
  mext.setBody(body->c_str());
  memoryOut->writeIntBigEndian(10);
  memoryOut->write(body->c_str(), body->size());

  // 16 TOPIC
  memoryOut->writeByte(10);
  memoryOut->write(body->c_str(), body->size());
  mext.setTopic(body->c_str());

  // 17 PROPERTIES
  memoryOut->writeShortBigEndian(0);

  mext.setMsgId(MQDecoder::createMessageId(mext.getStoreHost(), (int64)mext.getCommitLogOffset()));

  vector<MQMessageExt> mqvec;
  MemoryBlock block = memoryOut->getMemoryBlock();
  MQDecoder::decodes(&block, mqvec);
  EXPECT_EQ(mqvec.size(), 1);
  std::cout << mext.toString() << "\n";
  std::cout << mqvec[0].toString() << "\n";
  EXPECT_EQ(mqvec[0].toString(), mext.toString());

  mqvec.clear();
  MQDecoder::decodes(&block, mqvec, false);
  EXPECT_FALSE(mqvec[0].getBody().size());

  //===============================================================
  // 8 SYSFLAG  40=36+4
  mext.setSysFlag(0 | MessageSysFlag::CompressedFlag);
  memoryOut->setPosition(36);
  memoryOut->writeIntBigEndian(mext.getSysFlag());

  // 15 Body 84
  string outBody;
  string boody("123123123");
  UtilAll::deflate(boody, outBody, 5);
  mext.setBody(outBody);

  memoryOut->setPosition(84);
  memoryOut->writeIntBigEndian(outBody.size());
  memoryOut->write(outBody.c_str(), outBody.size());

  // 16 TOPIC
  memoryOut->writeByte(10);
  memoryOut->write(body->c_str(), body->size());
  mext.setTopic(body->c_str());

  // 17 PROPERTIES
  map<string, string> properties;
  properties["RocketMQ"] = "cpp-client";
  properties[MQMessage::PROPERTY_UNIQ_CLIENT_MESSAGE_ID_KEYIDX] = "123456";
  mext.setProperties(properties);
  mext.setMsgId("123456");

  string proString = MQDecoder::messageProperties2String(properties);

  memoryOut->writeShortBigEndian(proString.size());
  memoryOut->write(proString.c_str(), proString.size());

  mext.setStoreSize(memoryOut->getDataSize());
  memoryOut->setPosition(0);
  memoryOut->writeIntBigEndian(mext.getStoreSize());

  block = memoryOut->getMemoryBlock();
  MQDecoder::decodes(&block, mqvec);
  EXPECT_EQ(mqvec[0].toString(), mext.toString());
}

TEST(decoder, messagePropertiesAndToString) {
  map<string, string> properties;
  properties["RocketMQ"] = "cpp-client";
  string proString = MQDecoder::messageProperties2String(properties);

  map<string, string> newProperties;
  MQDecoder::string2messageProperties(proString, newProperties);
  EXPECT_EQ(properties, newProperties);
}

int main(int argc, char* argv[]) {
  InitGoogleMock(&argc, argv);

  testing::GTEST_FLAG(filter) = "decoder.*";
  int itestts = RUN_ALL_TESTS();
  return itestts;
}
