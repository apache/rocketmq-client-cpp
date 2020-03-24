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

#include <memory>
#include "map"
#include "string.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "json/value.h"
#include "json/writer.h"

#include "ClientRPCHook.h"
#include "ClientRemotingProcessor.h"
#include "ConsumerRunningInfo.h"
#include "MQClientFactory.h"
#include "MQMessageQueue.h"
#include "MQProtos.h"
#include "RemotingCommand.h"
#include "SessionCredentials.h"
#include "UtilAll.h"
#include "dataBlock.h"

using std::map;
using std::string;

using ::testing::_;
using ::testing::InitGoogleMock;
using ::testing::InitGoogleTest;
using testing::Mock;
using testing::Return;
using testing::SetArgReferee;

using Json::FastWriter;
using Json::Value;

using rocketmq::ClientRemotingProcessor;
using rocketmq::ClientRPCHook;
using rocketmq::ConsumerRunningInfo;
using rocketmq::GetConsumerRunningInfoRequestHeader;
using rocketmq::MemoryBlock;
using rocketmq::MQClientFactory;
using rocketmq::MQMessageQueue;
using rocketmq::MQRequestCode;
using rocketmq::MQResponseCode;
using rocketmq::NotifyConsumerIdsChangedRequestHeader;
using rocketmq::RemotingCommand;
using rocketmq::ResetOffsetBody;
using rocketmq::ResetOffsetRequestHeader;
using rocketmq::SessionCredentials;
using rocketmq::UtilAll;

class MockClientRemotingProcessor : public ClientRemotingProcessor {
 public:
  MockClientRemotingProcessor(MQClientFactory* factrory) : ClientRemotingProcessor(factrory) {}
  MOCK_METHOD1(resetOffset, RemotingCommand*(RemotingCommand* request));
  MOCK_METHOD1(getConsumerRunningInfo, RemotingCommand*(RemotingCommand* request));
  MOCK_METHOD1(notifyConsumerIdsChanged, RemotingCommand*(RemotingCommand* request));
};

class MockMQClientFactory : public MQClientFactory {
 public:
  MockMQClientFactory(const string& clientID,
                      int pullThreadNum,
                      uint64_t tcpConnectTimeout,
                      uint64_t tcpTransportTryLockTimeout,
                      string unitName)
      : MQClientFactory(clientID,
                        pullThreadNum,
                        tcpConnectTimeout,
                        tcpTransportTryLockTimeout,
                        unitName,
                        true,
                        rocketmq::DEFAULT_SSL_PROPERTY_FILE) {}

  MOCK_METHOD3(resetOffset,
               void(const string& group, const string& topic, const map<MQMessageQueue, int64>& offsetTable));
  MOCK_METHOD1(consumerRunningInfo, ConsumerRunningInfo*(const string& consumerGroup));
  MOCK_METHOD2(getSessionCredentialFromConsumer,
               bool(const string& consumerGroup, SessionCredentials& sessionCredentials));
  MOCK_METHOD1(doRebalanceByConsumerGroup, void(const string& consumerGroup));
};

TEST(clientRemotingProcessor, processRequest) {
  MockMQClientFactory* factory = new MockMQClientFactory("testClientId", 4, 3000, 4000, "a");
  ClientRemotingProcessor clientRemotingProcessor(factory);

  string addr = "127.0.0.1:9876";
  RemotingCommand* command = new RemotingCommand();
  RemotingCommand* pResponse = new RemotingCommand(13);

  pResponse->setCode(MQRequestCode::RESET_CONSUMER_CLIENT_OFFSET);
  command->setCode(MQRequestCode::RESET_CONSUMER_CLIENT_OFFSET);
  EXPECT_TRUE(clientRemotingProcessor.processRequest(addr, command) == nullptr);
  EXPECT_EQ(nullptr, clientRemotingProcessor.processRequest(addr, command));

  NotifyConsumerIdsChangedRequestHeader* header = new NotifyConsumerIdsChangedRequestHeader();
  header->setGroup("testGroup");
  RemotingCommand* twoCommand = new RemotingCommand(MQRequestCode::NOTIFY_CONSUMER_IDS_CHANGED, header);

  EXPECT_EQ(NULL, clientRemotingProcessor.processRequest(addr, twoCommand));

  command->setCode(MQRequestCode::GET_CONSUMER_RUNNING_INFO);
  // EXPECT_EQ(NULL , clientRemotingProcessor.processRequest(addr, command));

  command->setCode(MQRequestCode::CHECK_TRANSACTION_STATE);
  EXPECT_TRUE(clientRemotingProcessor.processRequest(addr, command) == nullptr);

  command->setCode(MQRequestCode::GET_CONSUMER_STATUS_FROM_CLIENT);
  EXPECT_TRUE(clientRemotingProcessor.processRequest(addr, command) == nullptr);

  command->setCode(MQRequestCode::CONSUME_MESSAGE_DIRECTLY);
  EXPECT_TRUE(clientRemotingProcessor.processRequest(addr, command) == nullptr);

  command->setCode(1);
  EXPECT_TRUE(clientRemotingProcessor.processRequest(addr, command) == nullptr);

  delete twoCommand;
  delete command;
  delete pResponse;
}

TEST(clientRemotingProcessor, resetOffset) {
  MockMQClientFactory* factory = new MockMQClientFactory("testClientId", 4, 3000, 4000, "a");
  Mock::AllowLeak(factory);
  ClientRemotingProcessor clientRemotingProcessor(factory);
  Value root;
  Value messageQueues;
  Value messageQueue;
  messageQueue["brokerName"] = "testBroker";
  messageQueue["queueId"] = 4;
  messageQueue["topic"] = "testTopic";
  messageQueue["offset"] = 1024;

  messageQueues.append(messageQueue);
  root["offsetTable"] = messageQueues;

  FastWriter wrtier;
  string strData = wrtier.write(root);

  ResetOffsetRequestHeader* header = new ResetOffsetRequestHeader();
  RemotingCommand* request = new RemotingCommand(13, header);

  EXPECT_CALL(*factory, resetOffset(_, _, _)).Times(1);
  clientRemotingProcessor.resetOffset(request);

  request->SetBody(strData.c_str(), strData.size() - 2);
  clientRemotingProcessor.resetOffset(request);

  request->SetBody(strData.c_str(), strData.size());
  clientRemotingProcessor.resetOffset(request);

  // here header no need delete, it will managered by RemotingCommand
  // delete header;
  delete request;
}

TEST(clientRemotingProcessor, getConsumerRunningInfoFailed) {
  MockMQClientFactory* factory = new MockMQClientFactory("testClientId", 4, 3000, 4000, "a");
  Mock::AllowLeak(factory);
  ConsumerRunningInfo* info = new ConsumerRunningInfo();
  EXPECT_CALL(*factory, consumerRunningInfo(_)).Times(2).WillOnce(Return(info)).WillOnce(Return(info));
  EXPECT_CALL(*factory, getSessionCredentialFromConsumer(_, _))
      .Times(2);  //.WillRepeatedly(SetArgReferee<1>(sessionCredentials));
  ClientRemotingProcessor clientRemotingProcessor(factory);

  GetConsumerRunningInfoRequestHeader* header = new GetConsumerRunningInfoRequestHeader();
  header->setConsumerGroup("testGroup");
  header->setClientId("testClientId");
  header->setJstackEnable(false);

  RemotingCommand* request = new RemotingCommand(14, header);

  RemotingCommand* command = clientRemotingProcessor.getConsumerRunningInfo("127.0.0.1:9876", request);
  EXPECT_EQ(command->getCode(), MQResponseCode::SYSTEM_ERROR);
  EXPECT_EQ(command->getRemark(), "The Consumer Group not exist in this consumer");
  delete command;
  delete request;
}

TEST(clientRemotingProcessor, notifyConsumerIdsChanged) {
  MockMQClientFactory* factory = new MockMQClientFactory("testClientId", 4, 3000, 4000, "a");
  Mock::AllowLeak(factory);
  ClientRemotingProcessor clientRemotingProcessor(factory);
  NotifyConsumerIdsChangedRequestHeader* header = new NotifyConsumerIdsChangedRequestHeader();
  header->setGroup("testGroup");
  RemotingCommand* request = new RemotingCommand(14, header);

  EXPECT_CALL(*factory, doRebalanceByConsumerGroup(_)).Times(1);
  clientRemotingProcessor.notifyConsumerIdsChanged(request);

  delete request;
}

TEST(clientRemotingProcessor, resetOffsetBody) {
  MockMQClientFactory* factory = new MockMQClientFactory("testClientId", 4, 3000, 4000, "a");
  ClientRemotingProcessor clientRemotingProcessor(factory);

  Value root;
  Value messageQueues;
  Value messageQueue;
  messageQueue["brokerName"] = "testBroker";
  messageQueue["queueId"] = 4;
  messageQueue["topic"] = "testTopic";
  messageQueue["offset"] = 1024;

  messageQueues.append(messageQueue);
  root["offsetTable"] = messageQueues;

  FastWriter wrtier;
  string strData = wrtier.write(root);

  MemoryBlock* mem = new MemoryBlock(strData.c_str(), strData.size());

  ResetOffsetBody* resetOffset = ResetOffsetBody::Decode(mem);

  map<MQMessageQueue, int64> map = resetOffset->getOffsetTable();
  MQMessageQueue mqmq("testTopic", "testBroker", 4);
  EXPECT_EQ(map[mqmq], 1024);
  Mock::AllowLeak(factory);
  delete resetOffset;
  delete mem;
}

int main(int argc, char* argv[]) {
  InitGoogleMock(&argc, argv);
  testing::GTEST_FLAG(throw_on_failure) = true;
  testing::GTEST_FLAG(filter) = "clientRemotingProcessor.*";
  int itestts = RUN_ALL_TESTS();
  return itestts;
}
