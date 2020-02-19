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

#include "UtilAll.h"

using std::string;

using ::testing::InitGoogleMock;
using ::testing::InitGoogleTest;
using testing::Return;

using rocketmq::UtilAll;

TEST(UtilAll, startsWith_retry) {
  string source = "testTopic";
  string retrySource = "%RETRY%testTopic";
  string noRetrySource = "%DLQ%testTopic";
  EXPECT_TRUE(UtilAll::startsWith_retry(retrySource));
  EXPECT_FALSE(UtilAll::startsWith_retry(source));
  EXPECT_FALSE(UtilAll::startsWith_retry(noRetrySource));
}
TEST(UtilAll, getRetryTopic) {
  string source = "testTopic";
  string retrySource = "%RETRY%testTopic";
  EXPECT_EQ(UtilAll::getRetryTopic(source), retrySource);
}
TEST(UtilAll, Trim) {
  string source = "testTopic";
  string preSource = "  testTopic";
  string surSource = "testTopic  ";
  string allSource = "  testTopic  ";
  UtilAll::Trim(preSource);
  UtilAll::Trim(surSource);
  UtilAll::Trim(allSource);
  EXPECT_EQ(preSource, source);
  EXPECT_EQ(surSource, source);
  EXPECT_EQ(allSource, source);
}
TEST(UtilAll, hexstr2ull) {
  const char* a = "1";
  const char* b = "FF";
  const char* c = "1a";
  const char* d = "101";
  EXPECT_EQ(UtilAll::hexstr2ull(a), 1);
  EXPECT_EQ(UtilAll::hexstr2ull(b), 255);
  EXPECT_EQ(UtilAll::hexstr2ull(c), 26);
  EXPECT_EQ(UtilAll::hexstr2ull(d), 257);
}
TEST(UtilAll, SplitURL) {
  string source = "127.0.0.1";
  string source1 = "127.0.0.1:0";
  string source2 = "127.0.0.1:9876";
  string addr;
  string addr1;
  string addr2;
  short port;
  EXPECT_FALSE(UtilAll::SplitURL(source, addr, port));
  EXPECT_FALSE(UtilAll::SplitURL(source1, addr1, port));
  EXPECT_TRUE(UtilAll::SplitURL(source2, addr2, port));
  EXPECT_EQ(addr2, "127.0.0.1");
  EXPECT_EQ(port, 9876);
}
TEST(UtilAll, SplitOne) {
  string source = "127.0.0.1:9876";
  vector<string> ret;
  EXPECT_EQ(UtilAll::Split(ret, source, '.'), 4);
  EXPECT_EQ(ret[0], "127");
}
TEST(UtilAll, SplitStr) {
  string source = "11AA222AA3333AA44444AA5";
  vector<string> ret;
  EXPECT_EQ(UtilAll::Split(ret, source, "AA"), 5);
  EXPECT_EQ(ret[0], "11");
}
TEST(UtilAll, StringToInt32) {
  string source = "123";
  int value;
  EXPECT_TRUE(UtilAll::StringToInt32(source, value));
  EXPECT_EQ(123, value);
  EXPECT_FALSE(UtilAll::StringToInt32("123456789X123456789", value));
  EXPECT_FALSE(UtilAll::StringToInt32("-1234567890123456789", value));
  EXPECT_FALSE(UtilAll::StringToInt32("1234567890123456789", value));
}
TEST(UtilAll, StringToInt64) {
  string source = "123";
  int64_t value;
  EXPECT_TRUE(UtilAll::StringToInt64(source, value));
  EXPECT_EQ(123, value);
  EXPECT_FALSE(UtilAll::StringToInt64("XXXXXXXXXXX", value));
  EXPECT_FALSE(UtilAll::StringToInt64("123456789X123456789", value));
  EXPECT_EQ(123456789, value);
  EXPECT_FALSE(UtilAll::StringToInt64("-123456789012345678901234567890123456789012345678901234567890", value));
  EXPECT_FALSE(UtilAll::StringToInt64("123456789012345678901234567890123456789012345678901234567890", value));
}
int main(int argc, char* argv[]) {
  InitGoogleMock(&argc, argv);
  testing::GTEST_FLAG(throw_on_failure) = true;
  testing::GTEST_FLAG(filter) = "UtilAll.*";
  int itestts = RUN_ALL_TESTS();
  return itestts;
}
