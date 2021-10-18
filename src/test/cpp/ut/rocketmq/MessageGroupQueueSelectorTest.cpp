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
#include "MessageGroupQueueSelector.h"
#include "rocketmq/MQMessage.h"
#include "rocketmq/MQMessageQueue.h"
#include "gtest/gtest.h"

ROCKETMQ_NAMESPACE_BEGIN

TEST(MessageGroupQueueSelectorTest, testSelect) {
  std::string message_group("sample");
  std::size_t hash_code = std::hash<std::string>{}(message_group);
  MessageGroupQueueSelector selector(message_group);
  std::size_t len = 8;
  std::vector<MQMessageQueue> mqs;
  mqs.resize(len);
  for (std::size_t i = 0; i < len; i++) {
    MQMessageQueue queue("topic", "broker-a", static_cast<int>(i));
    mqs.emplace_back(queue);
  }

  MQMessage message;
  MQMessageQueue selected = selector.select(mqs, message, nullptr);
  EXPECT_EQ(selected, mqs[hash_code % len]);
}

ROCKETMQ_NAMESPACE_END