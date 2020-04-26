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

#include <memory>

#include "MQDecoder.h"
#include "MQMessage.h"
#include "MessageBatch.h"

using testing::InitGoogleMock;
using testing::InitGoogleTest;
using testing::Return;

using rocketmq::MessageBatch;
using rocketmq::MQDecoder;
using rocketmq::MQMessage;

TEST(MessageBatchTest, Encode) {
  std::vector<MQMessage*> msgs;
  msgs.push_back(new MQMessage("topic", "*", "test1"));
  std::unique_ptr<MessageBatch> msgBatch(MessageBatch::generateFromList(msgs));
  auto encodeMessage = msgBatch->encode();
  auto encodeMessage2 = MQDecoder::encodeMessages(msgs);
  EXPECT_EQ(encodeMessage, encodeMessage2);
  // 20 + bodyLen(test1) + 2 + propertiesLength(TAGS:*;WAIT:true;);
  EXPECT_EQ(encodeMessage.size(), 44);

  msgs.push_back(new MQMessage("topic", "*", "test2"));
  msgs.push_back(new MQMessage("topic", "*", "test3"));
  msgBatch.reset(MessageBatch::generateFromList(msgs));
  encodeMessage = msgBatch->encode();
  encodeMessage2 = MQDecoder::encodeMessages(msgs);
  EXPECT_EQ(encodeMessage, encodeMessage2);
  EXPECT_EQ(encodeMessage.size(), 132);  // 44 * 3

  for (auto* msg : msgs) {
    delete msg;
  }
}

int main(int argc, char* argv[]) {
  InitGoogleMock(&argc, argv);
  testing::GTEST_FLAG(throw_on_failure) = true;
  testing::GTEST_FLAG(filter) = "MessageBatchTest.*";
  return RUN_ALL_TESTS();
}
