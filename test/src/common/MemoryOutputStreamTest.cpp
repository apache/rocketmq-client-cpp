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

#include "DataBlock.h"
#include "MemoryInputStream.h"
#include "MemoryOutputStream.h"

using testing::InitGoogleMock;
using testing::InitGoogleTest;
using testing::Return;

using rocketmq::MemoryBlock;
using rocketmq::MemoryInputStream;
using rocketmq::MemoryOutputStream;
using rocketmq::MemoryPool;

TEST(MemoryOutputStreamTest, Init) {
  MemoryOutputStream memoryOutput;
  EXPECT_EQ(memoryOutput.getMemoryBlock().getSize(), 0);
  EXPECT_TRUE(memoryOutput.getData() != nullptr);

  EXPECT_EQ(memoryOutput.getPosition(), 0);
  EXPECT_EQ(memoryOutput.getDataSize(), 0);

  MemoryOutputStream twoMemoryOutput(512);
  EXPECT_EQ(memoryOutput.getMemoryBlock().getSize(), 0);

  MemoryPool memoryBlock(12, false);
  MemoryOutputStream threeMemoryOutput(memoryBlock, false);
  EXPECT_EQ(threeMemoryOutput.getPosition(), 0);
  EXPECT_EQ(threeMemoryOutput.getDataSize(), 0);

  MemoryOutputStream frouMemoryOutput(memoryBlock, true);
  EXPECT_EQ(frouMemoryOutput.getPosition(), memoryBlock.getSize());
  EXPECT_EQ(frouMemoryOutput.getDataSize(), memoryBlock.getSize());

  char buf[] = "RocketMQ";
  MemoryOutputStream fiveMemoryOutputStream(buf, sizeof(buf));
  EXPECT_EQ(fiveMemoryOutputStream.getData(), buf);

  fiveMemoryOutputStream.reset();
  EXPECT_EQ(memoryOutput.getPosition(), 0);
  EXPECT_EQ(memoryOutput.getDataSize(), 0);
}

TEST(MemoryOutputStreamTest, Flush) {
  char buf[] = "RocketMQ";
  MemoryOutputStream memoryOutput;
  memoryOutput.write(buf, sizeof(buf) - 1);
  memoryOutput.flush();
  EXPECT_FALSE(memoryOutput.getData() == buf);
}

TEST(MemoryOutputStreamTest, Preallocate) {
  MemoryOutputStream memoryOutput;

  // TODO:
  memoryOutput.preallocate(250);

  memoryOutput.preallocate(256);
}

TEST(MemoryOutputStreamTest, GetMemoryBlock) {
  char buf[] = "RocketMQ";
  MemoryOutputStream memoryOutput;
  memoryOutput.write(buf, sizeof(buf) - 1);
  MemoryPool memoryBlock = memoryOutput.getMemoryBlock();
  EXPECT_EQ(memoryBlock.getSize(), memoryOutput.getDataSize());
}

TEST(MemoryOutputStreamTest, PrepareToWriteAndGetData) {
  char buf[] = "RocketMQ";
  MemoryOutputStream memoryOutput(buf, sizeof(buf));
  EXPECT_EQ(memoryOutput.getData(), buf);

  // prepareToWrite
  // EXPECT_TRUE(memoryOutput.writeIntBigEndian(123));
  EXPECT_TRUE(memoryOutput.writeByte('r'));
  EXPECT_STREQ(static_cast<const char*>(memoryOutput.getData()), "rocketMQ");

  MemoryOutputStream blockMmoryOutput(8);
  auto* memoryData = static_cast<const char*>(blockMmoryOutput.getData());
  EXPECT_EQ(memoryData[blockMmoryOutput.getDataSize()], 0);

  blockMmoryOutput.write(buf, 8);
  blockMmoryOutput.write(buf, 8);
  auto* data = static_cast<const char*>(blockMmoryOutput.getData());
  EXPECT_STREQ(data, "rocketMQrocketMQ");
}

TEST(MemoryOutputStreamTest, Position) {
  char buf[] = "RocketMQ";

  MemoryOutputStream memoryOutput;
  EXPECT_EQ(memoryOutput.getPosition(), 0);

  memoryOutput.write(buf, sizeof(buf) - 1);
  EXPECT_EQ(memoryOutput.getPosition(), 8);

  EXPECT_FALSE(memoryOutput.setPosition(9));

  EXPECT_TRUE(memoryOutput.setPosition(-1));
  EXPECT_EQ(memoryOutput.getPosition(), 0);

  EXPECT_TRUE(memoryOutput.setPosition(8));
  EXPECT_EQ(memoryOutput.getPosition(), 8);

  EXPECT_TRUE(memoryOutput.setPosition(7));
  EXPECT_EQ(memoryOutput.getPosition(), 7);
}

TEST(MemoryOutputStreamTest, Write) {
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
}

TEST(MemoryInputStreamTest, Info) {
  char buf[] = "RocketMQ";

  MemoryInputStream memoryInput(buf, sizeof(buf) - 1, false);
  EXPECT_EQ(memoryInput.getData(), buf);
  EXPECT_EQ(memoryInput.getTotalLength(), 8);

  MemoryInputStream twoMemoryInput(buf, sizeof(buf) - 1, true);
  EXPECT_NE(twoMemoryInput.getData(), buf);

  MemoryBlock memoryBlock(buf, sizeof(buf) - 1);
  MemoryInputStream threeMemoryInput(memoryBlock, false);
  EXPECT_EQ(threeMemoryInput.getData(), memoryBlock.getData());
  EXPECT_EQ(threeMemoryInput.getTotalLength(), 8);

  MemoryInputStream frouMemoryInput(memoryBlock, true);
  EXPECT_NE(frouMemoryInput.getData(), memoryBlock.getData());
}

TEST(MemoryInputStreamTest, Position) {
  char buf[] = "RocketMQ";

  MemoryInputStream memoryInput(buf, sizeof(buf) - 1, false);
  EXPECT_EQ(memoryInput.getPosition(), 0);
  EXPECT_FALSE(memoryInput.isExhausted());

  memoryInput.setPosition(9);
  EXPECT_EQ(memoryInput.getPosition(), 8);
  EXPECT_TRUE(memoryInput.isExhausted());

  memoryInput.setPosition(-1);
  EXPECT_EQ(memoryInput.getPosition(), 0);
}

int main(int argc, char* argv[]) {
  InitGoogleMock(&argc, argv);
  testing::GTEST_FLAG(throw_on_failure) = true;
  testing::GTEST_FLAG(filter) = "*.*";
  return RUN_ALL_TESTS();
}
