/*"
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

#include "MessageQueue.hpp"

using testing::InitGoogleMock;
using testing::InitGoogleTest;
using testing::Return;

using rocketmq::MQMessageQueue;
using rocketmq::toJson;

TEST(MessageQueueTest, Json) {
  MQMessageQueue messageQueue("testTopic", "testBroker", 1);
  EXPECT_EQ(messageQueue.queue_id(), 1);
  EXPECT_EQ(messageQueue.topic(), "testTopic");
  EXPECT_EQ(messageQueue.broker_name(), "testBroker");

  Json::Value outJson = toJson(messageQueue);
  EXPECT_EQ(outJson["queueId"], 1);
  EXPECT_EQ(outJson["topic"], "testTopic");
  EXPECT_EQ(outJson["brokerName"], "testBroker");
}

int main(int argc, char* argv[]) {
  InitGoogleMock(&argc, argv);
  testing::GTEST_FLAG(throw_on_failure) = true;
  testing::GTEST_FLAG(filter) = "MessageQueueTest.*";
  return RUN_ALL_TESTS();
}
