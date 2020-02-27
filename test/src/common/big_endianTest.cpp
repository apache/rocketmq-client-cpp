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
#include <stdlib.h>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "big_endian.h"

using ::testing::InitGoogleMock;
using ::testing::InitGoogleTest;
using testing::Return;

using rocketmq::BigEndianReader;
using rocketmq::BigEndianWriter;

TEST(big_endian, bigEndianObject) {
  char* buf = (char*)malloc(sizeof(char) * 32);

  BigEndianWriter writer(buf, 32);
  BigEndianReader reader(buf, 32);

  uint8_t* unit8 = (uint8_t*)malloc(sizeof(uint8_t));
  EXPECT_TRUE(writer.WriteU8((uint8_t)12));
  EXPECT_TRUE(reader.ReadU8(unit8));
  EXPECT_EQ(*unit8, 12);
  free(unit8);

  uint16_t* unit16 = (uint16_t*)malloc(sizeof(uint16_t));
  EXPECT_TRUE(writer.WriteU16((uint16_t)1200));
  EXPECT_TRUE(reader.ReadU16(unit16));
  EXPECT_EQ(*unit16, 1200);
  free(unit16);

  uint32_t* unit32 = (uint32_t*)malloc(sizeof(uint32_t));

  EXPECT_TRUE(writer.WriteU32((uint32_t)120000));
  EXPECT_TRUE(reader.ReadU32(unit32));
  EXPECT_EQ(*unit32, 120000);
  free(unit32);

  uint64_t* unit64 = (uint64_t*)malloc(sizeof(uint64_t));

  EXPECT_TRUE(writer.WriteU64((uint64_t)120000));
  EXPECT_TRUE(reader.ReadU64(unit64));
  EXPECT_EQ(*unit64, 120000);
  free(unit64);

  char* newBuf = (char*)malloc(sizeof(char) * 8);
  char* writeBuf = (char*)malloc(sizeof(char) * 8);
  strncpy(writeBuf, "RocketMQ", 8);
  EXPECT_TRUE(writer.WriteBytes(writeBuf, (size_t)8));
  EXPECT_TRUE(reader.ReadBytes(newBuf, (size_t)8));
  EXPECT_EQ(*writeBuf, *newBuf);

  free(newBuf);
  free(writeBuf);
}

TEST(big_endian, bigEndian) {
  char writeBuf[8];

  /*TODO
      char *newBuf = (char *) malloc(sizeof(char) * 8);
      strncpy(newBuf, "RocketMQ", 8);

      char readBuf[8];
      rocketmq::WriteBigEndian(writeBuf, newBuf);
      rocketmq::ReadBigEndian(writeBuf, readBuf);
      EXPECT_EQ(writeBuf, readBuf);
  */

  rocketmq::WriteBigEndian(writeBuf, (uint8_t)12);
  uint8_t* out = (uint8_t*)malloc(sizeof(uint8_t));
  rocketmq::ReadBigEndian(writeBuf, out);
  EXPECT_EQ(*out, 12);
  free(out);
}

int main(int argc, char* argv[]) {
  InitGoogleMock(&argc, argv);
  testing::GTEST_FLAG(throw_on_failure) = true;
  testing::GTEST_FLAG(filter) = "big_endian.*";
  int itestts = RUN_ALL_TESTS();
  return itestts;
}
