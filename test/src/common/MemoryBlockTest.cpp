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

#include "dataBlock.h"

using std::string;

using ::testing::InitGoogleMock;
using ::testing::InitGoogleTest;
using testing::Return;

using rocketmq::MemoryBlock;

TEST(memoryBlock, init) {
  MemoryBlock memoryBlock;
  EXPECT_EQ(memoryBlock.getSize(), 0);
  EXPECT_TRUE(memoryBlock.getData() == nullptr);

  MemoryBlock twoMemoryBlock(-1, true);
  EXPECT_EQ(twoMemoryBlock.getSize(), 0);
  EXPECT_TRUE(twoMemoryBlock.getData() == nullptr);

  MemoryBlock threeMemoryBlock(10, true);
  EXPECT_EQ(threeMemoryBlock.getSize(), 10);
  EXPECT_TRUE(threeMemoryBlock.getData() != nullptr);

  MemoryBlock frouMemoryBlock(12, false);
  EXPECT_EQ(frouMemoryBlock.getSize(), 12);
  EXPECT_TRUE(frouMemoryBlock.getData() != nullptr);

  char* buf = (char*)malloc(sizeof(char) * 9);
  strcpy(buf, "RocketMQ");
  MemoryBlock fiveMemoryBlock(buf, 0);
  EXPECT_EQ(fiveMemoryBlock.getSize(), 0);
  EXPECT_TRUE(fiveMemoryBlock.getData() == nullptr);

  char* bufNull = NULL;
  MemoryBlock sixMemoryBlock(bufNull, 16);
  EXPECT_EQ(sixMemoryBlock.getSize(), 16);
  EXPECT_TRUE(sixMemoryBlock.getData() != nullptr);

  buf = (char*)realloc(buf, 20);
  MemoryBlock sevenMemoryBlock(buf, 20);
  EXPECT_EQ(sevenMemoryBlock.getSize(), 20);
  sevenMemoryBlock.getData();
  EXPECT_EQ(string(sevenMemoryBlock.getData()), string(buf));

  MemoryBlock nineMemoryBlock(sevenMemoryBlock);
  EXPECT_EQ(nineMemoryBlock.getSize(), sevenMemoryBlock.getSize());
  EXPECT_EQ(string(nineMemoryBlock.getData()), string(sevenMemoryBlock.getData()));

  MemoryBlock eightMemoryBlock(fiveMemoryBlock);
  EXPECT_EQ(eightMemoryBlock.getSize(), fiveMemoryBlock.getSize());
  EXPECT_TRUE(eightMemoryBlock.getData() == nullptr);

  free(buf);
}

TEST(memoryBlock, operators) {
  MemoryBlock memoryBlock(12, false);

  MemoryBlock operaterMemoryBlock = memoryBlock;

  EXPECT_TRUE(operaterMemoryBlock == memoryBlock);

  char* buf = (char*)malloc(sizeof(char) * 16);
  memset(buf, 0, 16);
  strcpy(buf, "RocketMQ");
  MemoryBlock twoMemoryBlock(buf, 12);
  EXPECT_FALSE(memoryBlock == twoMemoryBlock);

  MemoryBlock threeMemoryBlock(buf, 16);
  EXPECT_FALSE(memoryBlock == threeMemoryBlock);
  EXPECT_TRUE(twoMemoryBlock != threeMemoryBlock);

  threeMemoryBlock.fillWith(49);
  EXPECT_EQ(string(threeMemoryBlock.getData(), 16), "1111111111111111");

  threeMemoryBlock.reset();
  EXPECT_EQ(threeMemoryBlock.getSize(), 0);
  EXPECT_TRUE(threeMemoryBlock.getData() == nullptr);

  threeMemoryBlock.setSize(16, 0);
  EXPECT_EQ(threeMemoryBlock.getSize(), 16);
  // EXPECT_EQ(threeMemoryBlock.getData() , buf);

  threeMemoryBlock.setSize(0, 0);
  EXPECT_EQ(threeMemoryBlock.getSize(), 0);
  EXPECT_TRUE(threeMemoryBlock.getData() == nullptr);

  MemoryBlock appendMemoryBlock;
  EXPECT_EQ(appendMemoryBlock.getSize(), 0);
  EXPECT_TRUE(appendMemoryBlock.getData() == nullptr);

  appendMemoryBlock.append(buf, -1);
  EXPECT_EQ(appendMemoryBlock.getSize(), 0);
  EXPECT_TRUE(appendMemoryBlock.getData() == nullptr);

  appendMemoryBlock.append(buf, 8);
  EXPECT_EQ(appendMemoryBlock.getSize(), 8);

  MemoryBlock replaceWithMemoryBlock;
  replaceWithMemoryBlock.append(buf, 8);

  char* aliyunBuf = (char*)malloc(sizeof(char) * 8);
  memset(aliyunBuf, 0, 8);
  strcpy(aliyunBuf, "aliyun");
  replaceWithMemoryBlock.replaceWith(aliyunBuf, 0);
  EXPECT_EQ(replaceWithMemoryBlock.getSize(), 8);
  EXPECT_EQ(string(replaceWithMemoryBlock.getData(), 8), "RocketMQ");

  replaceWithMemoryBlock.replaceWith(aliyunBuf, 6);
  EXPECT_EQ(replaceWithMemoryBlock.getSize(), 6);
  EXPECT_EQ(string(replaceWithMemoryBlock.getData(), strlen(aliyunBuf)), "aliyun");

  MemoryBlock insertMemoryBlock;
  insertMemoryBlock.append(buf, 8);
  insertMemoryBlock.insert(aliyunBuf, -1, -1);
  EXPECT_EQ(string(insertMemoryBlock.getData(), 8), "RocketMQ");

  /*    MemoryBlock fourInsertMemoryBlock;
   fourInsertMemoryBlock.append(buf , 8);
   // 6+ (-1)
   fourInsertMemoryBlock.insert(aliyunBuf , 8 , -1);
   string fourStr( fourInsertMemoryBlock.getData());
   EXPECT_TRUE( fourStr == "liyun");*/

  MemoryBlock twoInsertMemoryBlock;
  twoInsertMemoryBlock.append(buf, 8);
  twoInsertMemoryBlock.insert(aliyunBuf, strlen(aliyunBuf), 0);
  EXPECT_EQ(string(twoInsertMemoryBlock.getData(), 8 + strlen(aliyunBuf)), "aliyunRocketMQ");

  MemoryBlock threeInsertMemoryBlock;
  threeInsertMemoryBlock.append(buf, 8);
  threeInsertMemoryBlock.insert(aliyunBuf, 6, 100);
  EXPECT_EQ(string(threeInsertMemoryBlock.getData(), 8 + strlen(aliyunBuf)), "RocketMQaliyun");

  MemoryBlock removeSectionMemoryBlock(buf, 8);
  removeSectionMemoryBlock.removeSection(8, -1);
  EXPECT_EQ(string(removeSectionMemoryBlock.getData(), 8), "RocketMQ");

  MemoryBlock twoRemoveSectionMemoryBlock(buf, 8);
  twoRemoveSectionMemoryBlock.removeSection(1, 4);
  string str(twoRemoveSectionMemoryBlock.getData(), 4);
  EXPECT_TRUE(str == "RtMQ");

  free(buf);
  free(aliyunBuf);
}

int main(int argc, char* argv[]) {
  InitGoogleMock(&argc, argv);
  testing::GTEST_FLAG(throw_on_failure) = true;
  testing::GTEST_FLAG(filter) = "memoryBlock.*";
  int itestts = RUN_ALL_TESTS();
  return itestts;
}
