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
#include "BrokerData.h"

#include <iostream>

#include "rocketmq/RocketMQ.h"
#include "gtest/gtest.h"

#include "google/protobuf/struct.pb.h"

ROCKETMQ_NAMESPACE_BEGIN

TEST(BrokerDataTest, testDecode) {
  std::string json =
      R"({"brokerAddrs":{"1":"abc","2":"def"},"brokerName":"b1","cluster":"cluster","enableActingMaster":false})";
  google::protobuf::Struct root;

  auto status = google::protobuf::util::JsonStringToMessage(json, &root);
  ASSERT_TRUE(status.ok());

  BrokerData&& broker_data = BrokerData::decode(root);
  EXPECT_EQ("b1", broker_data.broker_name_);
  EXPECT_EQ("cluster", broker_data.cluster_);
  EXPECT_EQ(2, broker_data.broker_addresses_.size());
  EXPECT_EQ("abc", broker_data.broker_addresses_.at(1));
  EXPECT_EQ("def", broker_data.broker_addresses_.at(2));
}

ROCKETMQ_NAMESPACE_END