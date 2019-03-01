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
#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "UtilAll.h"

using ::testing::InitGoogleTest;
using ::testing::InitGoogleMock;
using testing::Return;

using rocketmq::UtilAll;

TEST(utilAll, retryTopic) {
    string retry = UtilAll::getRetryTopic("test");

    EXPECT_TRUE(UtilAll::startsWith_retry(retry));

    EXPECT_FALSE(UtilAll::startsWith_retry("test"));

}

TEST(utilAll, Trim) {
    string str = " 123 ";
    UtilAll::Trim(str);
    EXPECT_EQ(str, "123");

    UtilAll::Trim(str);
    EXPECT_EQ(str, "123");
}

TEST(utilAll, isBlank) {
    EXPECT_TRUE(UtilAll::isBlank(string()));

    EXPECT_TRUE(UtilAll::isBlank("blank\t"));
    EXPECT_TRUE(UtilAll::isBlank("blank\n"));
    EXPECT_TRUE(UtilAll::isBlank("blank\r"));

    EXPECT_FALSE(UtilAll::isBlank("blank"));
}

TEST(utilAll, hexstr2ull) {

}

TEST(utilAll, str2ll) {

}

TEST(utilAll, bytes2string) {

}

TEST(utilAll, to_bool) {

}

TEST(utilAll, SplitURL) {
    string addr;
    short nPort;
    EXPECT_FALSE(UtilAll::SplitURL("127.0.0.1", addr, nPort));

    EXPECT_TRUE(UtilAll::SplitURL("localhost:9876", addr, nPort));
    EXPECT_EQ(addr, "127.0.0.1");
    EXPECT_EQ(nPort, 9876);

    EXPECT_FALSE(UtilAll::SplitURL("localhost:abc", addr, nPort));
}

TEST(utilAll, Split) {
    vector<string> ret;
    string strIn = "RocketMQ::openMessage::aliyun";
    UtilAll::Split(ret, "RocketMQ::openMessage::aliyun", "::");
    EXPECT_EQ(ret.size(), 3);
    ret.clear();
    UtilAll::Split(ret, "RocketMQ:openMessage:aliyun", ':');
    EXPECT_EQ(ret.size(), 3);
}

TEST(utilAll, StringToInt32) {

}

TEST(utilAll, StringToInt64) {

}

TEST(utilAll, getLocalHostName) {

}

TEST(utilAll, getLocalAddress) {

}

TEST(utilAll, getHomeDirectory) {

}

TEST(utilAll, getProcessName) {

}

TEST(utilAll, currentTimeMillis) {

}

TEST(utilAll, currentTimeSeconds) {

}

TEST(utilAll, deflate) {

}

TEST(utilAll, inflate) {

}

TEST(utilAll, ReplaceFile) {

}

int main(int argc, char* argv[]) {
    InitGoogleMock(&argc, argv);
    testing::GTEST_FLAG(throw_on_failure) = true;
    testing::GTEST_FLAG(filter) = "messageExt.init";
    int itestts = RUN_ALL_TESTS();
    return itestts;
}
