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

#include "big_endian.h"

using testing::InitGoogleMock;
using testing::InitGoogleTest;
using testing::Return;

using rocketmq::BigEndianReader;
using rocketmq::BigEndianWriter;

TEST(BigEndianTest, BigEndianObject) {
  char buf[32];

  BigEndianWriter writer(buf, 32);
  BigEndianReader reader(buf, 32);

  uint64_t uint64[1];
  EXPECT_TRUE(writer.WriteU64((uint64_t)0x1234567890ABCDEF));
  EXPECT_EQ(*(uint64_t*)reader.ptr(), 0xEFCDAB9078563412);
  EXPECT_TRUE(reader.ReadU64(uint64));
  EXPECT_EQ(*uint64, 0x1234567890ABCDEF);

  uint32_t uint32[1];
  EXPECT_TRUE(writer.WriteU32((uint32_t)0x12345678));
  EXPECT_EQ(*(uint32_t*)reader.ptr(), 0x78563412);
  EXPECT_TRUE(reader.ReadU32(uint32));
  EXPECT_EQ(*uint32, 0x12345678);

  uint16_t uint16[1];
  EXPECT_TRUE(writer.WriteU16((uint16_t)0x1234));
  EXPECT_EQ(*(uint16_t*)reader.ptr(), 0x3412);
  EXPECT_TRUE(reader.ReadU16(uint16));
  EXPECT_EQ(*uint16, 0x1234);

  uint8_t uint8[1];
  EXPECT_TRUE(writer.WriteU8((uint8_t)0x12));
  EXPECT_EQ(*(uint8_t*)reader.ptr(), 0x12);
  EXPECT_TRUE(reader.ReadU8(uint8));
  EXPECT_EQ(*uint8, 0x12);

  char str[] = "RocketMQ";
  char str2[sizeof(str)];
  EXPECT_TRUE(writer.WriteBytes(str, sizeof(str) - 1));
  EXPECT_TRUE(strncmp(reader.ptr(), str, sizeof(str) - 1) == 0);
  EXPECT_TRUE(reader.ReadBytes(str2, sizeof(str) - 1));
  EXPECT_TRUE(strncmp(str2, str, sizeof(str) - 1) == 0);
}

TEST(BigEndianTest, BigEndian) {
  char buf[8];

  rocketmq::WriteBigEndian(buf, (uint8_t)0x12);
  EXPECT_EQ(*(uint8_t*)buf, 0x12);

  uint8_t out[1];
  rocketmq::ReadBigEndian(buf, out);
  EXPECT_EQ(*out, 0x12);
}

int main(int argc, char* argv[]) {
  InitGoogleMock(&argc, argv);
  testing::GTEST_FLAG(throw_on_failure) = true;
  testing::GTEST_FLAG(filter) = "BigEndianTest.*";
  return RUN_ALL_TESTS();
}
