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

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "CommunicationMode.h"
#include "MQClientAPIImpl.h"
#include "MQClientException.h"

using namespace std;
using namespace rocketmq;
using rocketmq::CommunicationMode;
using rocketmq::RemotingCommand;
using rocketmq::TcpRemotingClient;
using testing::_;
using ::testing::InitGoogleMock;
using ::testing::InitGoogleTest;
using testing::Mock;
using testing::Return;

class MockTcpRemotingClient : public TcpRemotingClient {
 public:
  MockTcpRemotingClient(int pullThreadNum, uint64_t tcpConnectTimeout, uint64_t tcpTransportTryLockTimeout)
      : TcpRemotingClient(pullThreadNum, tcpConnectTimeout, tcpTransportTryLockTimeout) {}

  MOCK_METHOD3(invokeSync, RemotingCommand*(const string&, RemotingCommand&, int));
};
class MockMQClientAPIImpl : public MQClientAPIImpl {
 public:
  MockMQClientAPIImpl(const string& mqClientId,
                      ClientRemotingProcessor* clientRemotingProcessor,
                      int pullThreadNum,
                      uint64_t tcpConnectTimeout,
                      uint64_t tcpTransportTryLockTimeout,
                      string unitName)
      : MQClientAPIImpl(mqClientId,
                        clientRemotingProcessor,
                        pullThreadNum,
                        tcpConnectTimeout,
                        tcpTransportTryLockTimeout,
                        unitName) {
    m_processor = clientRemotingProcessor;
  }
  ClientRemotingProcessor* m_processor;
  void reInitRemoteClient(TcpRemotingClient* client) {
    m_pRemotingClient.reset(client);
    m_pRemotingClient->registerProcessor(CHECK_TRANSACTION_STATE, m_processor);
    m_pRemotingClient->registerProcessor(RESET_CONSUMER_CLIENT_OFFSET, m_processor);
    m_pRemotingClient->registerProcessor(GET_CONSUMER_STATUS_FROM_CLIENT, m_processor);
    m_pRemotingClient->registerProcessor(GET_CONSUMER_RUNNING_INFO, m_processor);
    m_pRemotingClient->registerProcessor(NOTIFY_CONSUMER_IDS_CHANGED, m_processor);
    m_pRemotingClient->registerProcessor(CONSUME_MESSAGE_DIRECTLY, m_processor);
  }
};
class MockMQClientAPIImplUtil {
 public:
  static MockMQClientAPIImplUtil* GetInstance() {
    static MockMQClientAPIImplUtil instance;
    return &instance;
  }
  MockMQClientAPIImpl* GetGtestMockClientAPIImpl() {
    if (m_impl != nullptr) {
      return m_impl;
    }
    string cid = "testClientId";
    int ptN = 1;
    uint64_t tct = 3000;
    uint64_t ttt = 3000;
    string un = "central";
    SessionCredentials sc;
    ClientRemotingProcessor* pp = new ClientRemotingProcessor(nullptr);
    MockMQClientAPIImpl* impl = new MockMQClientAPIImpl(cid, pp, ptN, tct, ttt, un);
    MockTcpRemotingClient* pClient = new MockTcpRemotingClient(ptN, tct, ttt);
    impl->reInitRemoteClient(pClient);
    m_impl = impl;
    m_pClient = pClient;
    return impl;
  }
  MockTcpRemotingClient* GetGtestMockRemotingClient() { return m_pClient; }
  MockMQClientAPIImpl* m_impl = nullptr;
  MockTcpRemotingClient* m_pClient = nullptr;
};

TEST(MQClientAPIImplTest, getMaxOffset) {
  SessionCredentials sc;
  MockMQClientAPIImpl* impl = MockMQClientAPIImplUtil::GetInstance()->GetGtestMockClientAPIImpl();
  Mock::AllowLeak(impl);
  MockTcpRemotingClient* pClient = MockMQClientAPIImplUtil::GetInstance()->GetGtestMockRemotingClient();
  Mock::AllowLeak(pClient);
  GetMaxOffsetResponseHeader* pHead = new GetMaxOffsetResponseHeader();
  pHead->offset = 4096;
  RemotingCommand* pCommandFailed = new RemotingCommand(SYSTEM_ERROR, nullptr);
  RemotingCommand* pCommandSuccuss = new RemotingCommand(SUCCESS_VALUE, pHead);
  EXPECT_CALL(*pClient, invokeSync(_, _, _))
      .Times(3)
      .WillOnce(Return(nullptr))
      .WillOnce(Return(pCommandFailed))
      .WillOnce(Return(pCommandSuccuss));
  EXPECT_ANY_THROW(impl->getMaxOffset("127.0.0.0:10911", "testTopic", 0, 1000, sc));
  EXPECT_ANY_THROW(impl->getMaxOffset("127.0.0.0:10911", "testTopic", 0, 1000, sc));
  int64 offset = impl->getMaxOffset("127.0.0.0:10911", "testTopic", 0, 1000, sc);
  EXPECT_EQ(4096, offset);
}

TEST(MQClientAPIImplTest, getMinOffset) {
  SessionCredentials sc;
  MockMQClientAPIImpl* impl = MockMQClientAPIImplUtil::GetInstance()->GetGtestMockClientAPIImpl();
  Mock::AllowLeak(impl);
  MockTcpRemotingClient* pClient = MockMQClientAPIImplUtil::GetInstance()->GetGtestMockRemotingClient();
  Mock::AllowLeak(pClient);
  GetMinOffsetResponseHeader* pHead = new GetMinOffsetResponseHeader();
  pHead->offset = 2048;
  RemotingCommand* pCommandFailed = new RemotingCommand(SYSTEM_ERROR, nullptr);
  RemotingCommand* pCommandSuccuss = new RemotingCommand(SUCCESS_VALUE, pHead);
  EXPECT_CALL(*pClient, invokeSync(_, _, _))
      .Times(3)
      .WillOnce(Return(nullptr))
      .WillOnce(Return(pCommandFailed))
      .WillOnce(Return(pCommandSuccuss));
  EXPECT_ANY_THROW(impl->getMinOffset("127.0.0.0:10911", "testTopic", 0, 1000, sc));
  EXPECT_ANY_THROW(impl->getMinOffset("127.0.0.0:10911", "testTopic", 0, 1000, sc));
  int64 offset = impl->getMinOffset("127.0.0.0:10911", "testTopic", 0, 1000, sc);
  EXPECT_EQ(2048, offset);
}

TEST(MQClientAPIImplTest, sendMessage) {
  string cid = "testClientId";
  SessionCredentials sc;
  MockMQClientAPIImpl* impl = MockMQClientAPIImplUtil::GetInstance()->GetGtestMockClientAPIImpl();
  Mock::AllowLeak(impl);
  MockTcpRemotingClient* pClient = MockMQClientAPIImplUtil::GetInstance()->GetGtestMockRemotingClient();
  Mock::AllowLeak(pClient);

  SendMessageResponseHeader* pHead = new SendMessageResponseHeader();
  pHead->msgId = "MessageID";
  pHead->queueId = 1;
  pHead->queueOffset = 409600;
  RemotingCommand* pCommandSync = new RemotingCommand(SUCCESS_VALUE, pHead);
  EXPECT_CALL(*pClient, invokeSync(_, _, _)).Times(1).WillOnce(Return(pCommandSync));
  MQMessage message("testTopic", "Hello, RocketMQ");
  string unique_msgId = "UniqMessageID";
  message.setProperty(MQMessage::PROPERTY_UNIQ_CLIENT_MESSAGE_ID_KEYIDX, unique_msgId);
  SendMessageRequestHeader* requestHeader = new SendMessageRequestHeader();
  requestHeader->producerGroup = cid;
  requestHeader->topic = (message.getTopic());
  requestHeader->defaultTopic = DEFAULT_TOPIC;
  requestHeader->defaultTopicQueueNums = 4;
  requestHeader->bornTimestamp = UtilAll::currentTimeMillis();
  SendResult result =
      impl->sendMessage("127.0.0.0:10911", "testBroker", message, requestHeader, 100, 1, ComMode_SYNC, nullptr, sc);
  EXPECT_EQ(result.getSendStatus(), SEND_OK);
  EXPECT_EQ(result.getMsgId(), unique_msgId);
  EXPECT_EQ(result.getQueueOffset(), 409600);
  EXPECT_EQ(result.getOffsetMsgId(), "MessageID");
  EXPECT_EQ(result.getMessageQueue().getBrokerName(), "testBroker");
  EXPECT_EQ(result.getMessageQueue().getTopic(), "testTopic");
}

TEST(MQClientAPIImplTest, consumerSendMessageBack) {
  SessionCredentials sc;
  MQMessageExt msg;
  MockMQClientAPIImpl* impl = MockMQClientAPIImplUtil::GetInstance()->GetGtestMockClientAPIImpl();
  Mock::AllowLeak(impl);
  MockTcpRemotingClient* pClient = MockMQClientAPIImplUtil::GetInstance()->GetGtestMockRemotingClient();
  Mock::AllowLeak(pClient);
  RemotingCommand* pCommandFailed = new RemotingCommand(SYSTEM_ERROR, nullptr);
  RemotingCommand* pCommandSuccuss = new RemotingCommand(SUCCESS_VALUE, nullptr);
  EXPECT_CALL(*pClient, invokeSync(_, _, _))
      .Times(3)
      .WillOnce(Return(nullptr))
      .WillOnce(Return(pCommandFailed))
      .WillOnce(Return(pCommandSuccuss));
  EXPECT_ANY_THROW(impl->consumerSendMessageBack("127.0.0.0:10911", msg, "testGroup", 0, 1000, 16, sc));
  EXPECT_ANY_THROW(impl->consumerSendMessageBack("127.0.0.0:10911", msg, "testGroup", 0, 1000, 16, sc));
  EXPECT_NO_THROW(impl->consumerSendMessageBack("127.0.0.0:10911", msg, "testGroup", 0, 1000, 16, sc));
}

int main(int argc, char* argv[]) {
  InitGoogleMock(&argc, argv);
  testing::GTEST_FLAG(filter) = "MQClientAPIImplTest.*";
  return RUN_ALL_TESTS();
}
