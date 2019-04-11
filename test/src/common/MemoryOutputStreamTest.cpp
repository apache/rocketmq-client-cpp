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
#include "string.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "MemoryInputStream.h"
#include "MemoryOutputStream.h"
#include "dataBlock.h"

using std::string;

using ::testing::InitGoogleMock;
using ::testing::InitGoogleTest;
using testing::Return;

using rocketmq::MemoryBlock;
using rocketmq::MemoryInputStream;
using rocketmq::MemoryOutputStream;

TEST(memoryOutputStream, init) {
  MemoryOutputStream memoryOutput;
  EXPECT_EQ(memoryOutput.getMemoryBlock().getSize(), 0);
  EXPECT_TRUE(memoryOutput.getData() != nullptr);

  EXPECT_EQ(memoryOutput.getPosition(), 0);
  EXPECT_EQ(memoryOutput.getDataSize(), 0);

  MemoryOutputStream twoMemoryOutput(512);
  EXPECT_EQ(memoryOutput.getMemoryBlock().getSize(), 0);

  MemoryBlock memoryBlock(12, false);
  MemoryOutputStream threeMemoryOutput(memoryBlock, false);
  EXPECT_EQ(threeMemoryOutput.getPosition(), 0);
  EXPECT_EQ(threeMemoryOutput.getDataSize(), 0);

  MemoryOutputStream frouMemoryOutput(memoryBlock, true);
  EXPECT_EQ(frouMemoryOutput.getPosition(), memoryBlock.getSize());
  EXPECT_EQ(frouMemoryOutput.getDataSize(), memoryBlock.getSize());

  char* buf = (char*)malloc(sizeof(char) * 9);
  strcpy(buf, "RocketMQ");
  MemoryOutputStream fiveMemoryOutputStream(buf, 8);
  EXPECT_EQ(fiveMemoryOutputStream.getData(), buf);

  fiveMemoryOutputStream.reset();
  EXPECT_EQ(memoryOutput.getPosition(), 0);
  EXPECT_EQ(memoryOutput.getDataSize(), 0);
  free(buf);
}

TEST(memoryOutputStream, flush) {
  char* buf = (char*)malloc(sizeof(char) * 9);
  strcpy(buf, "RocketMQ");
  MemoryOutputStream memoryOutput;
  memoryOutput.write(buf, 9);
  memoryOutput.flush();
  EXPECT_FALSE(memoryOutput.getData() == buf);
  free(buf);
}

TEST(memoryOutputStream, preallocate) {
  MemoryOutputStream memoryOutput;
  memoryOutput.preallocate(250);

  memoryOutput.preallocate(256);
}

TEST(memoryOutputStream, getMemoryBlock) {
  char* buf = (char*)malloc(sizeof(char) * 9);
  strcpy(buf, "RocketMQ");
  MemoryOutputStream memoryOutput;
  memoryOutput.write(buf, 9);
  MemoryBlock memoryBlock = memoryOutput.getMemoryBlock();
  EXPECT_EQ(memoryBlock.getSize(), memoryOutput.getDataSize());
  free(buf);
}

TEST(memoryOutputStream, prepareToWriteAndGetData) {
  char* buf = (char*)malloc(sizeof(char) * 9);
  strcpy(buf, "RocketMQ");
  MemoryOutputStream memoryOutput(buf, 9);
  EXPECT_EQ(memoryOutput.getData(), buf);

  // prepareToWrite
  // EXPECT_TRUE(memoryOutput.writeIntBigEndian(123));
  EXPECT_TRUE(memoryOutput.writeByte('r'));
  const char* data = static_cast<const char*>(memoryOutput.getData());
  EXPECT_EQ(string(data), "rocketMQ");

  MemoryOutputStream blockMmoryOutput(8);
  char* memoryData = (char*)blockMmoryOutput.getData();

  EXPECT_EQ(memoryData[blockMmoryOutput.getDataSize()], 0);

  blockMmoryOutput.write(buf, 8);
  blockMmoryOutput.write(buf, 8);
  data = static_cast<const char*>(blockMmoryOutput.getData());
  EXPECT_EQ(string(data), "rocketMQrocketMQ");
  free(buf);
}

TEST(memoryOutputStream, position) {
  char* buf = (char*)malloc(sizeof(char) * 9);
  strcpy(buf, "RocketMQ");

  MemoryOutputStream memoryOutput;
  EXPECT_EQ(memoryOutput.getPosition(), 0);

  memoryOutput.write(buf, 8);
  EXPECT_EQ(memoryOutput.getPosition(), 8);

  EXPECT_FALSE(memoryOutput.setPosition(9));

  EXPECT_TRUE(memoryOutput.setPosition(-1));
  EXPECT_EQ(memoryOutput.getPosition(), 0);

  EXPECT_TRUE(memoryOutput.setPosition(8));
  EXPECT_EQ(memoryOutput.getPosition(), 8);

  EXPECT_TRUE(memoryOutput.setPosition(7));
  EXPECT_EQ(memoryOutput.getPosition(), 7);
  free(buf);
}

TEST(memoryOutputStream, write) {
  MemoryOutputStream memoryOutput;
  MemoryInputStream memoryInput(memoryOutput.getData(), 256, false);

  EXPECT_TRUE(memoryOutput.writeBool(true));
  EXPECT_TRUE(memoryInput.readBool());

  EXPECT_TRUE(memoryOutput.writeBool(false));
  EXPECT_FALSE(memoryInput.readBool());

  EXPECT_TRUE(memoryOutput.writeByte('a'));
  EXPECT_EQ(memoryInput.readByte(), 'a');

  EXPECT_TRUE(memoryOutput.writeShortBigEndian(128));
  EXPECT_EQ(memoryInput.readShortBigEndian(), 128);

  EXPECT_TRUE(memoryOutput.writeIntBigEndian(123));
  EXPECT_EQ(memoryInput.readIntBigEndian(), 123);

  EXPECT_TRUE(memoryOutput.writeInt64BigEndian(123123));
  EXPECT_EQ(memoryInput.readInt64BigEndian(), 123123);

  EXPECT_TRUE(memoryOutput.writeDoubleBigEndian(12.71));
  EXPECT_EQ(memoryInput.readDoubleBigEndian(), 12.71);

  EXPECT_TRUE(memoryOutput.writeFloatBigEndian(12.1));
  float f = 12.1;
  EXPECT_EQ(memoryInput.readFloatBigEndian(), f);

  // EXPECT_TRUE(memoryOutput.writeRepeatedByte(8 , 8));
}

TEST(memoryInputStream, info) {
  char* buf = (char*)malloc(sizeof(char) * 9);
  strcpy(buf, "RocketMQ");

  MemoryInputStream memoryInput(buf, 8, false);

  char* memoryData = (char*)memoryInput.getData();
  EXPECT_EQ(memoryData, buf);

  EXPECT_EQ(memoryInput.getTotalLength(), 8);

  MemoryInputStream twoMemoryInput(buf, 8, true);
  EXPECT_NE(twoMemoryInput.getData(), buf);

  memoryData = (char*)twoMemoryInput.getData();
  EXPECT_NE(&memoryData, &buf);

  MemoryBlock memoryBlock(buf, 8);
  MemoryInputStream threeMemoryInput(memoryBlock, false);
  memoryData = (char*)threeMemoryInput.getData();
  EXPECT_EQ(memoryData, threeMemoryInput.getData());
  EXPECT_EQ(threeMemoryInput.getTotalLength(), 8);

  MemoryInputStream frouMemoryInput(memoryBlock, true);
  EXPECT_NE(frouMemoryInput.getData(), memoryBlock.getData());
  free(buf);
}

TEST(memoryInputStream, position) {
  char* buf = (char*)malloc(sizeof(char) * 9);
  strcpy(buf, "RocketMQ");

  MemoryInputStream memoryInput(buf, 8, false);
  EXPECT_EQ(memoryInput.getPosition(), 0);
  EXPECT_FALSE(memoryInput.isExhausted());

  memoryInput.setPosition(9);
  EXPECT_EQ(memoryInput.getPosition(), 8);
  EXPECT_TRUE(memoryInput.isExhausted());

  memoryInput.setPosition(-1);
  EXPECT_EQ(memoryInput.getPosition(), 0);
  free(buf);
}

int main(int argc, char* argv[]) {
  InitGoogleMock(&argc, argv);
  testing::GTEST_FLAG(throw_on_failure) = true;
  testing::GTEST_FLAG(filter) = "*.*";
  int itestts = RUN_ALL_TESTS();
  return itestts;
}
