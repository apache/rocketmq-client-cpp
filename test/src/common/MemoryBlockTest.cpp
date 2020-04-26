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

#include <string>

#include "DataBlock.h"

using testing::InitGoogleMock;
using testing::InitGoogleTest;
using testing::Return;

using rocketmq::MemoryBlock;
using rocketmq::MemoryPool;

TEST(MemoryBlockTest, Init) {
  MemoryBlock memoryBlock;
  EXPECT_EQ(memoryBlock.getSize(), 0);
  EXPECT_TRUE(memoryBlock.getData() == nullptr);

  MemoryPool twoMemoryBlock(0, true);
  EXPECT_EQ(twoMemoryBlock.getSize(), 0);
  EXPECT_TRUE(twoMemoryBlock.getData() == nullptr);

  MemoryPool threeMemoryBlock(10, true);
  EXPECT_EQ(threeMemoryBlock.getSize(), 10);
  EXPECT_TRUE(threeMemoryBlock.getData() != nullptr);

  MemoryPool frouMemoryBlock(12, false);
  EXPECT_EQ(frouMemoryBlock.getSize(), 12);
  EXPECT_TRUE(frouMemoryBlock.getData() != nullptr);

  char buf[] = "RocketMQ";
  MemoryPool fiveMemoryBlock(buf, 0);
  EXPECT_EQ(fiveMemoryBlock.getSize(), 0);
  EXPECT_TRUE(fiveMemoryBlock.getData() == nullptr);

  MemoryPool sixMemoryBlock(nullptr, 16);
  EXPECT_EQ(sixMemoryBlock.getSize(), 16);
  EXPECT_TRUE(sixMemoryBlock.getData() != nullptr);

  MemoryPool sevenMemoryBlock(buf, sizeof(buf) - 1);
  EXPECT_EQ(sevenMemoryBlock.getSize(), sizeof(buf) - 1);
  EXPECT_EQ((std::string)sevenMemoryBlock, buf);

  MemoryPool nineMemoryBlock(sevenMemoryBlock);
  EXPECT_EQ(nineMemoryBlock.getSize(), sevenMemoryBlock.getSize());
  EXPECT_EQ(nineMemoryBlock, sevenMemoryBlock);

  MemoryPool eightMemoryBlock(fiveMemoryBlock);
  EXPECT_EQ(eightMemoryBlock.getSize(), fiveMemoryBlock.getSize());
  EXPECT_TRUE(eightMemoryBlock.getData() == nullptr);
}

TEST(MemoryBlockTest, Operators) {
  MemoryPool memoryBlock(8, false);

  MemoryPool operaterMemoryBlock = memoryBlock;
  EXPECT_TRUE(operaterMemoryBlock == memoryBlock);

  char buf[] = "RocketMQ";
  MemoryPool twoMemoryBlock(buf, sizeof(buf) - 1);
  EXPECT_FALSE(memoryBlock == twoMemoryBlock);

  MemoryPool threeMemoryBlock(buf, 7);
  EXPECT_FALSE(memoryBlock == threeMemoryBlock);
  EXPECT_TRUE(twoMemoryBlock != threeMemoryBlock);

  threeMemoryBlock.fillWith(49);
  EXPECT_EQ((std::string)threeMemoryBlock, "1111111");

  threeMemoryBlock.reset();
  EXPECT_EQ(threeMemoryBlock.getSize(), 0);
  EXPECT_TRUE(threeMemoryBlock.getData() == nullptr);

  threeMemoryBlock.setSize(16, false);
  EXPECT_EQ(threeMemoryBlock.getSize(), 16);

  threeMemoryBlock.setSize(0, false);
  EXPECT_EQ(threeMemoryBlock.getSize(), 0);
  EXPECT_TRUE(threeMemoryBlock.getData() == nullptr);

  MemoryPool appendMemoryBlock;
  EXPECT_EQ(appendMemoryBlock.getSize(), 0);
  EXPECT_TRUE(appendMemoryBlock.getData() == nullptr);

  appendMemoryBlock.append(buf, 0);
  EXPECT_EQ(appendMemoryBlock.getSize(), 0);
  EXPECT_TRUE(appendMemoryBlock.getData() == nullptr);

  appendMemoryBlock.append(buf, sizeof(buf) - 1);
  EXPECT_EQ(appendMemoryBlock.getSize(), 8);

  MemoryPool replaceWithMemoryBlock;
  replaceWithMemoryBlock.append(buf, sizeof(buf) - 1);

  char aliyunBuf[] = "aliyun";
  replaceWithMemoryBlock.replaceWith(aliyunBuf, 0);
  EXPECT_EQ(replaceWithMemoryBlock.getSize(), 8);
  EXPECT_EQ((std::string)replaceWithMemoryBlock, "RocketMQ");

  replaceWithMemoryBlock.replaceWith(aliyunBuf, sizeof(aliyunBuf) - 1);
  EXPECT_EQ(replaceWithMemoryBlock.getSize(), 6);
  EXPECT_EQ((std::string)replaceWithMemoryBlock, "aliyun");

  MemoryPool insertMemoryBlock;
  insertMemoryBlock.append(buf, sizeof(buf) - 1);
  insertMemoryBlock.insert(aliyunBuf, 0, 0);
  EXPECT_EQ((std::string)insertMemoryBlock, "RocketMQ");

  MemoryPool twoInsertMemoryBlock;
  twoInsertMemoryBlock.append(buf, sizeof(buf) - 1);
  twoInsertMemoryBlock.insert(aliyunBuf, sizeof(aliyunBuf) - 1, 0);
  EXPECT_EQ((std::string)twoInsertMemoryBlock, "aliyunRocketMQ");

  MemoryPool threeInsertMemoryBlock;
  threeInsertMemoryBlock.append(buf, sizeof(buf) - 1);
  threeInsertMemoryBlock.insert(aliyunBuf, sizeof(aliyunBuf) - 1, 100);
  EXPECT_EQ((std::string)threeInsertMemoryBlock, "RocketMQaliyun");

  MemoryPool removeSectionMemoryBlock(buf, sizeof(buf) - 1);
  removeSectionMemoryBlock.removeSection(8, 0);
  EXPECT_EQ((std::string)removeSectionMemoryBlock, "RocketMQ");

  MemoryPool twoRemoveSectionMemoryBlock(buf, sizeof(buf) - 1);
  twoRemoveSectionMemoryBlock.removeSection(1, 4);
  EXPECT_EQ((std::string)twoRemoveSectionMemoryBlock, "RtMQ");
}

int main(int argc, char* argv[]) {
  InitGoogleMock(&argc, argv);
  testing::GTEST_FLAG(throw_on_failure) = true;
  testing::GTEST_FLAG(filter) = "MemoryBlockTest.*";
  return RUN_ALL_TESTS();
}
