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

#include "QueueData.h"
#include "rocketmq/RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

TEST(QueueDataTest, testDecode) {
  std::string json = R"({"brokerName":"broker1","perm":6,"readQueueNums":4,"topicSynFlag":3,"writeQueueNums":8})";
  google::protobuf::Struct root;
  auto status = google::protobuf::util::JsonStringToMessage(json, &root);
  EXPECT_TRUE(status.ok());

  QueueData queue_data = QueueData::decode(root);
  EXPECT_EQ("broker1", queue_data.broker_name_);
  EXPECT_EQ(6, queue_data.perm_);
  EXPECT_EQ(4, queue_data.read_queue_number_);
  EXPECT_EQ(8, queue_data.write_queue_number_);
  EXPECT_EQ(3, queue_data.topic_system_flag_);
}

ROCKETMQ_NAMESPACE_END