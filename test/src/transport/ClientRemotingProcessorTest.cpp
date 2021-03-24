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
#include "ClientRemotingProcessor.h"

#include <map>
#include <memory>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <json/value.h>
#include <json/writer.h>

#include "ByteArray.h"
#include "ClientRPCHook.h"
#include "MQClientConfigImpl.hpp"
#include "MQClientInstance.h"
#include "MQMessageQueue.h"
#include "MQProtos.h"
#include "RemotingCommand.h"
#include "SessionCredentials.h"
#include "TcpTransport.h"
#include "UtilAll.h"
#include "protocol/body/ConsumerRunningInfo.h"
#include "protocol/body/ResetOffsetBody.hpp"
#include "protocol/header/CommandHeader.h"

using testing::_;
using testing::InitGoogleMock;
using testing::InitGoogleTest;
using testing::Mock;
using testing::Return;
using testing::SetArgReferee;

using Json::FastWriter;
using Json::Value;

using rocketmq::ByteArray;
using rocketmq::ClientRemotingProcessor;
using rocketmq::ClientRPCHook;
using rocketmq::ConsumerRunningInfo;
using rocketmq::GetConsumerRunningInfoRequestHeader;
using rocketmq::MQClientConfig;
using rocketmq::MQClientConfigImpl;
using rocketmq::MQClientInstance;
using rocketmq::MQMessageQueue;
using rocketmq::MQRequestCode;
using rocketmq::MQResponseCode;
using rocketmq::NotifyConsumerIdsChangedRequestHeader;
using rocketmq::RemotingCommand;
using rocketmq::ResetOffsetBody;
using rocketmq::ResetOffsetRequestHeader;
using rocketmq::SessionCredentials;
using rocketmq::TcpTransport;
using rocketmq::UtilAll;

class MockClientRemotingProcessor : public ClientRemotingProcessor {
 public:
  MockClientRemotingProcessor(MQClientInstance* instance) : ClientRemotingProcessor(instance) {}

  MOCK_METHOD(RemotingCommand*, resetOffset, (RemotingCommand*), ());
  MOCK_METHOD(RemotingCommand*, getConsumerRunningInfo, (RemotingCommand*), ());
  MOCK_METHOD(RemotingCommand*, notifyConsumerIdsChanged, (RemotingCommand*), ());
};

class MockMQClientInstance : public MQClientInstance {
 public:
  MockMQClientInstance(const MQClientConfig& clientConfig, const std::string& clientId)
      : MQClientInstance(clientConfig, clientId) {}

  MOCK_METHOD(void,
              resetOffset,
              (const std::string&, const std::string&, (const std::map<MQMessageQueue, int64_t>&)),
              ());
  MOCK_METHOD(ConsumerRunningInfo*, consumerRunningInfo, (const std::string&), ());
  MOCK_METHOD(void, doRebalanceByConsumerGroup, (const std::string&), ());
};

// TEST(ClientRemotingProcessorTest, ProcessRequest) {
//   MQClientConfigImpl clientConfig;
//   clientConfig.setUnitName("a");
//   clientConfig.setTcpTransportWorkerThreadNum(4);
//   clientConfig.setTcpTransportConnectTimeout(3000);
//   clientConfig.setTcpTransportTryLockTimeout(4000);
//   std::unique_ptr<MockMQClientInstance> instance(new MockMQClientInstance(clientConfig, "testClientId"));
//   ClientRemotingProcessor clientRemotingProcessor(instance.get());

//   auto channel = TcpTransport::CreateTransport(nullptr, nullptr, nullptr);
//   std::string addr = "127.0.0.1:9876";

//   std::unique_ptr<RemotingCommand> requestCommand(new RemotingCommand());

//   // reset offset request without body
//   requestCommand->setCode(MQRequestCode::RESET_CONSUMER_CLIENT_OFFSET);
//   EXPECT_EQ(nullptr, clientRemotingProcessor.processRequest(channel, requestCommand.get()));

//   auto* header = new NotifyConsumerIdsChangedRequestHeader();
//   header->setConsumerGroup("testGroup");
//   RemotingCommand* twoCommand = new RemotingCommand(MQRequestCode::NOTIFY_CONSUMER_IDS_CHANGED, header);

//   EXPECT_EQ(NULL, clientRemotingProcessor.processRequest(addr, twoCommand));

//   command->setCode(MQRequestCode::GET_CONSUMER_RUNNING_INFO);
//   // EXPECT_EQ(NULL , clientRemotingProcessor.processRequest(addr, command));

//   command->setCode(MQRequestCode::CHECK_TRANSACTION_STATE);
//   EXPECT_TRUE(clientRemotingProcessor.processRequest(addr, command) == nullptr);

//   command->setCode(MQRequestCode::GET_CONSUMER_STATUS_FROM_CLIENT);
//   EXPECT_TRUE(clientRemotingProcessor.processRequest(addr, command) == nullptr);

//   command->setCode(MQRequestCode::CONSUME_MESSAGE_DIRECTLY);
//   EXPECT_TRUE(clientRemotingProcessor.processRequest(addr, command) == nullptr);

//   command->setCode(1);
//   EXPECT_TRUE(clientRemotingProcessor.processRequest(addr, command) == nullptr);

//   delete twoCommand;
//   delete command;
//   delete pResponse;
// }

// TEST(ClientRemotingProcessorTest, resetOffset) {
//   MockMQClientFactory* factory = new MockMQClientFactory("testClientId", 4, 3000, 4000, "a");
//   Mock::AllowLeak(factory);
//   ClientRemotingProcessor clientRemotingProcessor(factory);
//   Value root;
//   Value messageQueues;
//   Value messageQueue;
//   messageQueue["brokerName"] = "testBroker";
//   messageQueue["queueId"] = 4;
//   messageQueue["topic"] = "testTopic";
//   messageQueue["offset"] = 1024;

//   messageQueues.append(messageQueue);
//   root["offsetTable"] = messageQueues;

//   FastWriter wrtier;
//   string strData = wrtier.write(root);

//   ResetOffsetRequestHeader* header = new ResetOffsetRequestHeader();
//   RemotingCommand* request = new RemotingCommand(13, header);

//   EXPECT_CALL(*factory, resetOffset(_, _, _)).Times(1);
//   clientRemotingProcessor.resetOffset(request);

//   request->SetBody(strData.c_str(), strData.size() - 2);
//   clientRemotingProcessor.resetOffset(request);

//   request->SetBody(strData.c_str(), strData.size());
//   clientRemotingProcessor.resetOffset(request);

//   // here header no need delete, it will managered by RemotingCommand
//   // delete header;
//   delete request;
// }

// TEST(ClientRemotingProcessorTest, getConsumerRunningInfo) {
//   MockMQClientFactory* factory = new MockMQClientFactory("testClientId", 4, 3000, 4000, "a");
//   ConsumerRunningInfo* info = new ConsumerRunningInfo();
//   EXPECT_CALL(*factory, consumerRunningInfo(_)).Times(2).WillOnce(Return(info)).WillOnce(Return(info));
//   EXPECT_CALL(*factory, getSessionCredentialFromConsumer(_, _))
//       .Times(2);  //.WillRepeatedly(SetArgReferee<1>(sessionCredentials));
//   ClientRemotingProcessor clientRemotingProcessor(factory);

//   GetConsumerRunningInfoRequestHeader* header = new GetConsumerRunningInfoRequestHeader();
//   header->setConsumerGroup("testGroup");

//   RemotingCommand* request = new RemotingCommand(14, header);

//   RemotingCommand* command = clientRemotingProcessor.getConsumerRunningInfo("127.0.0.1:9876", request);
//   EXPECT_EQ(command->getCode(), MQResponseCode::SYSTEM_ERROR);
//   EXPECT_EQ(command->getRemark(), "The Consumer Group not exist in this consumer");
//   delete command;
//   delete request;
// }

// TEST(ClientRemotingProcessorTest, notifyConsumerIdsChanged) {
//   MockMQClientFactory* factory = new MockMQClientFactory("testClientId", 4, 3000, 4000, "a");
//   Mock::AllowLeak(factory);
//   ClientRemotingProcessor clientRemotingProcessor(factory);
//   NotifyConsumerIdsChangedRequestHeader* header = new NotifyConsumerIdsChangedRequestHeader();
//   header->setGroup("testGroup");
//   RemotingCommand* request = new RemotingCommand(14, header);

//   EXPECT_CALL(*factory, doRebalanceByConsumerGroup(_)).Times(1);
//   clientRemotingProcessor.notifyConsumerIdsChanged(request);

//   delete request;
// }

// TEST(ClientRemotingProcessorTest, resetOffsetBody) {
//   MockMQClientFactory* factory = new MockMQClientFactory("testClientId", 4, 3000, 4000, "a");
//   ClientRemotingProcessor clientRemotingProcessor(factory);

//   Value root;
//   Value messageQueues;
//   Value messageQueue;
//   messageQueue["brokerName"] = "testBroker";
//   messageQueue["queueId"] = 4;
//   messageQueue["topic"] = "testTopic";
//   messageQueue["offset"] = 1024;

//   messageQueues.append(messageQueue);
//   root["offsetTable"] = messageQueues;

//   FastWriter wrtier;
//   string strData = wrtier.write(root);

//   MemoryBlock* mem = new MemoryBlock(strData.c_str(), strData.size());

//   ResetOffsetBody* resetOffset = ResetOffsetBody::Decode(mem);

//   map<MQMessageQueue, int64> map = resetOffset->getOffsetTable();
//   MQMessageQueue mqmq("testTopic", "testBroker", 4);
//   EXPECT_EQ(map[mqmq], 1024);
//   Mock::AllowLeak(factory);
//   delete resetOffset;
//   delete mem;
// }

int main(int argc, char* argv[]) {
  InitGoogleMock(&argc, argv);
  testing::GTEST_FLAG(throw_on_failure) = true;
  testing::GTEST_FLAG(filter) = "ClientRemotingProcessorTest.*";
  return RUN_ALL_TESTS();
}
