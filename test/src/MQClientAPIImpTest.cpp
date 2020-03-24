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
  MockTcpRemotingClient() : TcpRemotingClient(true, DEFAULT_SSL_PROPERTY_FILE) {}

  MOCK_METHOD3(invokeSync, RemotingCommand*(const string&, RemotingCommand&, int));
  MOCK_METHOD6(invokeAsync, bool(const string&, RemotingCommand&, std::shared_ptr<AsyncCallbackWrap>, int64, int, int));
};
class MockMQClientAPIImpl : public MQClientAPIImpl {
 public:
  MockMQClientAPIImpl(const string& mqClientId,
                      ClientRemotingProcessor* clientRemotingProcessor,
                      int pullThreadNum,
                      uint64_t tcpConnectTimeout,
                      uint64_t tcpTransportTryLockTimeout,
                      string unitName)
      : MQClientAPIImpl(mqClientId, true, DEFAULT_SSL_PROPERTY_FILE) {}
  void reInitRemoteClient(TcpRemotingClient* client) { m_pRemotingClient.reset(client); }
};

TEST(MQClientAPIImplTest, getMaxOffset) {
  SessionCredentials sc;
  MockMQClientAPIImpl* impl = new MockMQClientAPIImpl("testMockAPIImpl", nullptr, 1, 2, 3, "testUnit");
  Mock::AllowLeak(impl);
  MockTcpRemotingClient* pClient = new MockTcpRemotingClient();
  Mock::AllowLeak(pClient);
  impl->reInitRemoteClient(pClient);
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
  MockMQClientAPIImpl* impl = new MockMQClientAPIImpl("testMockAPIImpl", nullptr, 1, 2, 3, "testUnit");
  Mock::AllowLeak(impl);
  MockTcpRemotingClient* pClient = new MockTcpRemotingClient();
  Mock::AllowLeak(pClient);
  impl->reInitRemoteClient(pClient);
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
class MyMockAutoDeleteSendCallback : public AutoDeleteSendCallBack {
 public:
  virtual ~MyMockAutoDeleteSendCallback() {}
  virtual void onSuccess(SendResult& sendResult) {
    std::cout << "send Success" << std::endl;
    return;
  }
  virtual void onException(MQException& e) {
    std::cout << "send Exception" << e << std::endl;
    return;
  }
};

TEST(MQClientAPIImplTest, sendMessage) {
  string cid = "testClientId";
  SessionCredentials sc;
  MockMQClientAPIImpl* impl = new MockMQClientAPIImpl("testMockAPIImpl", nullptr, 1, 2, 3, "testUnit");
  Mock::AllowLeak(impl);
  MockTcpRemotingClient* pClient = new MockTcpRemotingClient();
  Mock::AllowLeak(pClient);
  impl->reInitRemoteClient(pClient);
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

  // Try to test Async send

  EXPECT_CALL(*pClient, invokeAsync(_, _, _, _, _, _))
      .Times(7)
      .WillOnce(Return(false))
      .WillOnce(Return(true))
      .WillOnce(Return(false))
      .WillOnce(Return(true))
      .WillOnce(Return(false))
      .WillOnce(Return(false))
      .WillOnce(Return(false));

  SendMessageRequestHeader* requestHeader2 = new SendMessageRequestHeader();
  requestHeader2->producerGroup = cid;
  requestHeader2->topic = (message.getTopic());
  requestHeader2->defaultTopic = DEFAULT_TOPIC;
  requestHeader2->defaultTopicQueueNums = 4;
  requestHeader2->bornTimestamp = UtilAll::currentTimeMillis();
  EXPECT_ANY_THROW(
      impl->sendMessage("127.0.0.0:10911", "testBroker", message, requestHeader2, 100, 1, ComMode_ASYNC, nullptr, sc));

  SendMessageRequestHeader* requestHeader3 = new SendMessageRequestHeader();
  requestHeader3->producerGroup = cid;
  requestHeader3->topic = (message.getTopic());
  requestHeader3->defaultTopic = DEFAULT_TOPIC;
  requestHeader3->defaultTopicQueueNums = 4;
  requestHeader3->bornTimestamp = UtilAll::currentTimeMillis();
  SendCallback* pSendCallback = new MyMockAutoDeleteSendCallback();
  EXPECT_NO_THROW(impl->sendMessage("127.0.0.0:10911", "testBroker", message, requestHeader3, 100, 1, ComMode_ASYNC,
                                    pSendCallback, sc));

  SendMessageRequestHeader* requestHeader4 = new SendMessageRequestHeader();
  requestHeader4->producerGroup = cid;
  requestHeader4->topic = (message.getTopic());
  requestHeader4->defaultTopic = DEFAULT_TOPIC;
  requestHeader4->defaultTopicQueueNums = 4;
  requestHeader4->bornTimestamp = UtilAll::currentTimeMillis();
  SendCallback* pSendCallback2 = new MyMockAutoDeleteSendCallback();
  EXPECT_NO_THROW(impl->sendMessage("127.0.0.0:10911", "testBroker", message, requestHeader4, 1000, 2, ComMode_ASYNC,
                                    pSendCallback2, sc));

  SendMessageRequestHeader* requestHeader5 = new SendMessageRequestHeader();
  requestHeader5->producerGroup = cid;
  requestHeader5->topic = (message.getTopic());
  requestHeader5->defaultTopic = DEFAULT_TOPIC;
  requestHeader5->defaultTopicQueueNums = 4;
  requestHeader5->bornTimestamp = UtilAll::currentTimeMillis();
  SendCallback* pSendCallback3 = new MyMockAutoDeleteSendCallback();
  EXPECT_NO_THROW(impl->sendMessage("127.0.0.0:10911", "testBroker", message, requestHeader5, 1000, 3, ComMode_ASYNC,
                                    pSendCallback3, sc));
}

TEST(MQClientAPIImplTest, consumerSendMessageBack) {
  SessionCredentials sc;
  MQMessageExt msg;
  MockMQClientAPIImpl* impl = new MockMQClientAPIImpl("testMockAPIImpl", nullptr, 1, 2, 3, "testUnit");
  Mock::AllowLeak(impl);
  MockTcpRemotingClient* pClient = new MockTcpRemotingClient();
  Mock::AllowLeak(pClient);
  impl->reInitRemoteClient(pClient);
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
