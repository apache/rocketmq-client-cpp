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

#include "NameSpaceUtil.h"

using std::string;

using ::testing::InitGoogleMock;
using ::testing::InitGoogleTest;
using testing::Return;

using rocketmq::NameSpaceUtil;

TEST(NameSpaceUtil, isEndPointURL) {
  const string url = "http://rocketmq.nameserver.com";
  EXPECT_TRUE(NameSpaceUtil::isEndPointURL(url));
  EXPECT_FALSE(NameSpaceUtil::isEndPointURL("rocketmq.nameserver.com"));
  EXPECT_FALSE(NameSpaceUtil::isEndPointURL("127.0.0.1"));
}
TEST(NameSpaceUtil, formatNameServerURL) {
  string url = "http://rocketmq.nameserver.com";
  string urlFormatted = "rocketmq.nameserver.com";
  EXPECT_EQ(NameSpaceUtil::formatNameServerURL(url), urlFormatted);
  EXPECT_EQ(NameSpaceUtil::formatNameServerURL(urlFormatted), urlFormatted);
}
TEST(NameSpaceUtil, getNameSpaceFromNsURL) {
  string url = "http://MQ_INST_UNITTEST.rocketmq.nameserver.com";
  string url2 = "MQ_INST_UNITTEST.rocketmq.nameserver.com";
  string noInstUrl = "http://rocketmq.nameserver.com";
  string inst = "MQ_INST_UNITTEST";
  EXPECT_EQ(NameSpaceUtil::getNameSpaceFromNsURL(url), inst);
  EXPECT_EQ(NameSpaceUtil::getNameSpaceFromNsURL(url2), inst);
  EXPECT_EQ(NameSpaceUtil::getNameSpaceFromNsURL(noInstUrl), "");
}
TEST(NameSpaceUtil, checkNameSpaceExistInNsURL) {
  string url = "http://MQ_INST_UNITTEST.rocketmq.nameserver.com";
  string url2 = "MQ_INST_UNITTEST.rocketmq.nameserver.com";
  string noInstUrl = "http://rocketmq.nameserver.com";
  EXPECT_TRUE(NameSpaceUtil::checkNameSpaceExistInNsURL(url));
  EXPECT_FALSE(NameSpaceUtil::checkNameSpaceExistInNsURL(url2));
  EXPECT_FALSE(NameSpaceUtil::checkNameSpaceExistInNsURL(noInstUrl));
}
TEST(NameSpaceUtil, checkNameSpaceExistInNameServer) {
  string url = "http://MQ_INST_UNITTEST.rocketmq.nameserver.com";
  string url2 = "MQ_INST_UNITTEST.rocketmq.nameserver.com";
  string noInstUrl = "rocketmq.nameserver.com";
  string nsIP = "127.0.0.1";
  EXPECT_TRUE(NameSpaceUtil::checkNameSpaceExistInNameServer(url));
  EXPECT_TRUE(NameSpaceUtil::checkNameSpaceExistInNameServer(url2));
  EXPECT_FALSE(NameSpaceUtil::checkNameSpaceExistInNameServer(noInstUrl));
  EXPECT_FALSE(NameSpaceUtil::checkNameSpaceExistInNameServer(nsIP));
}
TEST(NameSpaceUtil, withNameSpace) {
  string source = "testTopic";
  string ns = "MQ_INST_UNITTEST";
  string nsSource = "MQ_INST_UNITTEST%testTopic";
  EXPECT_EQ(NameSpaceUtil::withNameSpace(source, ns), nsSource);
  EXPECT_EQ(NameSpaceUtil::withNameSpace(source, ""), source);
}
TEST(NameSpaceUtil, hasNameSpace) {
  string source = "testTopic";
  string ns = "MQ_INST_UNITTEST";
  string nsSource = "MQ_INST_UNITTEST%testTopic";
  string nsTraceSource = "rmq_sys_TRACE_DATA_Region";
  EXPECT_TRUE(NameSpaceUtil::hasNameSpace(nsSource, ns));
  EXPECT_FALSE(NameSpaceUtil::hasNameSpace(source, ns));
  EXPECT_FALSE(NameSpaceUtil::hasNameSpace(source, ""));
  EXPECT_TRUE(NameSpaceUtil::hasNameSpace(nsTraceSource, ns));
}
int main(int argc, char* argv[]) {
  InitGoogleMock(&argc, argv);
  testing::GTEST_FLAG(throw_on_failure) = true;
  testing::GTEST_FLAG(filter) = "NameSpaceUtil.*";
  int itestts = RUN_ALL_TESTS();
  return itestts;
}
