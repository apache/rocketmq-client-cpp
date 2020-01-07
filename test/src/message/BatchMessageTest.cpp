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
#include <unistd.h>
#include <stdio.h>
#include <iostream>
#include "BatchMessage.h"
#include "MQMessage.h"
#include <map>

using namespace std;
using namespace rocketmq;
using ::testing::InitGoogleTest;
using ::testing::InitGoogleMock;
using testing::Return;

TEST(BatchMessageEncodeTest, encodeMQMessage) {
  MQMessage msg1("topic", "*", "test");
  // const map<string,string>& properties = msg1.getProperties();
  // for (auto& pair : properties) {
  //    std::cout << pair.first << " : " << pair.second << std::endl;
  //}

  EXPECT_EQ(msg1.getProperties().size(), 2);
  EXPECT_EQ(msg1.getBody().size(), 4);
  // 20 + bodyLen + 2 + propertiesLength;
  string encodeMessage = BatchMessage::encode(msg1);
  EXPECT_EQ(encodeMessage.size(), 43);

  msg1.setProperty(MQMessage::PROPERTY_UNIQ_CLIENT_MESSAGE_ID_KEYIDX, "1");
  encodeMessage = BatchMessage::encode(msg1);
  EXPECT_EQ(encodeMessage.size(), 54);
}

TEST(BatchMessageEncodeTest, encodeMQMessages) {
  std::vector<MQMessage> msgs;
  MQMessage msg1("topic", "*", "test1");
  // const map<string,string>& properties = msg1.getProperties();
  // for (auto& pair : properties) {
  //    std::cout << pair.first << " : " << pair.second << std::endl;
  //}
  msgs.push_back(msg1);
  // 20 + bodyLen + 2 + propertiesLength;
  string encodeMessage = BatchMessage::encode(msgs);
  EXPECT_EQ(encodeMessage.size(), 86);
  MQMessage msg2("topic", "*", "test2");
  MQMessage msg3("topic", "*", "test3");
  msgs.push_back(msg2);
  msgs.push_back(msg3);
  encodeMessage = BatchMessage::encode(msgs);
  EXPECT_EQ(encodeMessage.size(), 258);  // 86*3
}

int main(int argc, char* argv[]) {
  InitGoogleMock(&argc, argv);
  return RUN_ALL_TESTS();
}
