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
#include "rocketmq/MQMessageExt.h"
#include "MessageAccessor.h"
#include "rocketmq/MQMessage.h"
#include "gtest/gtest.h"
#include <chrono>

ROCKETMQ_NAMESPACE_BEGIN

class MQMessageExtTest : public testing::Test {
public:
  void SetUp() override {
    MessageAccessor::setMessageId(message_, msg_id_);
  }

  void TearDown() override {
  }

protected:
  std::string msg_id_{"msg-0"};
  std::string topic_{"test"};
  MQMessageExt message_;
};

TEST_F(MQMessageExtTest, testGetQueueId) {
  EXPECT_EQ(message_.getQueueId(), 0);
}

TEST_F(MQMessageExtTest, testBornTimestamp) {
  auto born_timestamp = message_.bornTimestamp();
  EXPECT_TRUE(std::chrono::system_clock::now() - born_timestamp < std::chrono::seconds(1));
}

TEST_F(MQMessageExtTest, testGetDeliveryAttempt) {
  EXPECT_EQ(message_.getDeliveryAttempt(), 0);
}

TEST_F(MQMessageExtTest, testGetBornTimestamp) {
  int64_t born_timestamp = message_.getBornTimestamp();
  uint64_t last_second = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::system_clock::now().time_since_epoch() - std::chrono::seconds(1))
                             .count();
  EXPECT_TRUE(static_cast<uint64_t>(born_timestamp) > last_second);
}

TEST_F(MQMessageExtTest, testEqual) {
  MQMessageExt other;
  other.setTopic("test2");
  MessageAccessor::setMessageId(other, msg_id_);
  EXPECT_TRUE(message_ == other);
}

ROCKETMQ_NAMESPACE_END