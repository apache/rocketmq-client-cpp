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

#include "TraceConstant.h"
#include "TraceUtil.h"

using std::string;

using ::testing::InitGoogleMock;
using ::testing::InitGoogleTest;
using testing::Return;

using rocketmq::TraceBean;
using rocketmq::TraceConstant;
using rocketmq::TraceContext;
using rocketmq::TraceMessageType;
using rocketmq::TraceTransferBean;
using rocketmq::TraceType;
using rocketmq::TraceUtil;

TEST(TraceUtil, CovertTraceTypeToString) {
  EXPECT_EQ(TraceUtil::CovertTraceTypeToString(TraceType::Pub), TraceConstant::TRACE_TYPE_PUB);
  EXPECT_EQ(TraceUtil::CovertTraceTypeToString(TraceType::SubBefore), TraceConstant::TRACE_TYPE_BEFORE);
  EXPECT_EQ(TraceUtil::CovertTraceTypeToString(TraceType::SubAfter), TraceConstant::TRACE_TYPE_AFTER);
  EXPECT_EQ(TraceUtil::CovertTraceTypeToString((TraceType)5), TraceConstant::TRACE_TYPE_PUB);
}
TEST(TraceUtil, CovertTraceContextToTransferBean) {
  TraceContext context;
  TraceBean bean;
  bean.setMsgType(TraceMessageType::TRACE_NORMAL_MSG);
  bean.setMsgId("MessageID");
  bean.setKeys("MessageKey");
  context.setRegionId("region");
  context.setMsgType(TraceMessageType::TRACE_TRANS_COMMIT_MSG);
  context.setTraceType(TraceType::Pub);
  context.setGroupName("PubGroup");
  context.setCostTime(50);
  context.setStatus(true);
  context.setTraceBean(bean);
  context.setTraceBeanIndex(1);
  TraceTransferBean beanPub = TraceUtil::CovertTraceContextToTransferBean(&context);
  EXPECT_GT(beanPub.getTransKey().size(), 0);
  context.setTraceType(TraceType::SubBefore);
  TraceTransferBean beanBefore = TraceUtil::CovertTraceContextToTransferBean(&context);
  EXPECT_GT(beanBefore.getTransKey().size(), 0);

  context.setTraceType(TraceType::SubAfter);
  TraceTransferBean beanAfter = TraceUtil::CovertTraceContextToTransferBean(&context);
  EXPECT_GT(beanAfter.getTransKey().size(), 0);

  TraceContext contextFailed("testGroup");
  contextFailed.setMsgType(context.getMsgType());
  contextFailed.setTraceType((TraceType)5);
  contextFailed.setRequestId(context.getRegionId());
  contextFailed.setTimeStamp(context.getTimeStamp());
  contextFailed.setTraceBeanIndex(context.getTraceBeanIndex());
  TraceTransferBean beanWrong = TraceUtil::CovertTraceContextToTransferBean(&contextFailed);
  EXPECT_EQ(beanWrong.getTransKey().size(), 0);
}
int main(int argc, char* argv[]) {
  InitGoogleMock(&argc, argv);
  testing::GTEST_FLAG(throw_on_failure) = true;
  testing::GTEST_FLAG(filter) = "TraceUtil.*";
  int itestts = RUN_ALL_TESTS();
  return itestts;
}
