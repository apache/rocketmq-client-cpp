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

#include "TraceBean.h"

using std::string;

using ::testing::InitGoogleMock;
using ::testing::InitGoogleTest;
using testing::Return;

using rocketmq::TraceBean;
using rocketmq::TraceMessageType;

TEST(TraceBean, Init) {
  std::string m_topic("topic");
  std::string m_msgId("msgid");
  std::string m_offsetMsgId("offsetmsgid");
  std::string m_tags("tag");
  std::string m_keys("ksy");
  std::string m_storeHost("storehost");
  std::string m_clientHost("clienthost");
  TraceMessageType m_msgType = TraceMessageType::TRACE_NORMAL_MSG;
  long long m_storeTime = 100;
  int m_retryTimes = 2;
  int m_bodyLength = 1024;
  TraceBean bean;
  bean.setTopic(m_topic);
  bean.setMsgId(m_msgId);
  bean.setOffsetMsgId(m_offsetMsgId);
  bean.setTags(m_tags);
  bean.setKeys(m_keys);
  bean.setStoreHost(m_storeHost);
  bean.setClientHost(m_clientHost);
  bean.setMsgType(m_msgType);
  bean.setStoreTime(m_storeTime);
  bean.setRetryTimes(m_retryTimes);
  bean.setBodyLength(m_bodyLength);
  EXPECT_EQ(bean.getTopic(), m_topic);
  EXPECT_EQ(bean.getMsgId(), m_msgId);
  EXPECT_EQ(bean.getOffsetMsgId(), m_offsetMsgId);
  EXPECT_EQ(bean.getTags(), m_tags);
  EXPECT_EQ(bean.getKeys(), m_keys);
  EXPECT_EQ(bean.getStoreHost(), m_storeHost);
  EXPECT_EQ(bean.getClientHost(), m_clientHost);
  EXPECT_EQ(bean.getMsgType(), m_msgType);
  EXPECT_EQ(bean.getStoreTime(), m_storeTime);
  EXPECT_EQ(bean.getRetryTimes(), m_retryTimes);
  EXPECT_EQ(bean.getBodyLength(), m_bodyLength);
}

int main(int argc, char* argv[]) {
  InitGoogleMock(&argc, argv);
  testing::GTEST_FLAG(throw_on_failure) = true;
  testing::GTEST_FLAG(filter) = "TraceBean.*";
  int itestts = RUN_ALL_TESTS();
  return itestts;
}
