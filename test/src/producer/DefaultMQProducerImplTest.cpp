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

#include <iostream>
#include <map>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "DefaultMQProducerImpl.h"
#include "MQClientFactory.h"
#include "TopicPublishInfo.h"

using namespace std;
using namespace rocketmq;
using rocketmq::DefaultMQProducerImpl;
using rocketmq::MQClientAPIImpl;
using rocketmq::MQClientFactory;
using rocketmq::TopicPublishInfo;
using testing::_;
using ::testing::InitGoogleMock;
using ::testing::InitGoogleTest;
using testing::Return;

class MySendCallback : public SendCallback {
  virtual void onSuccess(SendResult& sendResult) {}
  virtual void onException(MQException& e) {}
};

class MyMessageQueueSelector : public MessageQueueSelector {
  virtual MQMessageQueue select(const std::vector<MQMessageQueue>& mqs, const MQMessage& msg, void* arg) {
    return MQMessageQueue("TestTopic", "BrokerA", 0);
  }
};
class MockMQClientFactory : public MQClientFactory {
 public:
  MockMQClientFactory(const string& mqClientId) : MQClientFactory(mqClientId, true, DEFAULT_SSL_PROPERTY_FILE) {}
  MOCK_METHOD0(start, void());
  MOCK_METHOD0(shutdown, void());
  MOCK_METHOD0(sendHeartbeatToAllBroker, void());
  MOCK_METHOD0(getMQClientAPIImpl, MQClientAPIImpl*());
  MOCK_METHOD1(registerProducer, bool(MQProducer*));
  MOCK_METHOD1(unregisterProducer, void(MQProducer*));
  MOCK_METHOD1(findBrokerAddressInPublish, string(const string&));
  MOCK_METHOD2(tryToFindTopicPublishInfo,
               boost::shared_ptr<TopicPublishInfo>(const string&, const SessionCredentials&));
};
class MockMQClientAPIImpl : public MQClientAPIImpl {
 public:
  MockMQClientAPIImpl() : MQClientAPIImpl("testMockMQClientAPIImpl", true, DEFAULT_SSL_PROPERTY_FILE) {}

  MOCK_METHOD9(sendMessage,
               SendResult(const string&,
                          const string&,
                          const MQMessage&,
                          SendMessageRequestHeader*,
                          int,
                          int,
                          int,
                          SendCallback*,
                          const SessionCredentials&));
};
TEST(DefaultMQProducerImplTest, init) {
  DefaultMQProducerImpl* impl = new DefaultMQProducerImpl("testMQProducerGroup");
  EXPECT_EQ(impl->getGroupName(), "testMQProducerGroup");
  impl->setUnitName("testUnit");
  EXPECT_EQ(impl->getUnitName(), "testUnit");
  impl->setTcpTransportPullThreadNum(64);
  EXPECT_EQ(impl->getTcpTransportPullThreadNum(), 64);
  impl->setTcpTransportConnectTimeout(2000);
  EXPECT_EQ(impl->getTcpTransportConnectTimeout(), 2000);
  impl->setTcpTransportTryLockTimeout(3000);
  // need fix the unit
  EXPECT_EQ(impl->getTcpTransportTryLockTimeout(), 3);
  impl->setRetryTimes4Async(4);
  EXPECT_EQ(impl->getRetryTimes4Async(), 4);
  impl->setRetryTimes(2);
  EXPECT_EQ(impl->getRetryTimes(), 2);
  impl->setSendMsgTimeout(1000);
  EXPECT_EQ(impl->getSendMsgTimeout(), 1000);
  impl->setCompressMsgBodyOverHowmuch(1024);
  EXPECT_EQ(impl->getCompressMsgBodyOverHowmuch(), 1024);
  impl->setCompressLevel(2);
  EXPECT_EQ(impl->getCompressLevel(), 2);
  impl->setMaxMessageSize(2048);
  EXPECT_EQ(impl->getMaxMessageSize(), 2048);

  impl->setNamesrvAddr("http://rocketmq.nameserver.com");
  EXPECT_EQ(impl->getNamesrvAddr(), "rocketmq.nameserver.com");
  impl->setNameSpace("MQ_INST_NAMESPACE_TEST");
  EXPECT_EQ(impl->getNameSpace(), "MQ_INST_NAMESPACE_TEST");
  impl->setMessageTrace(true);
  EXPECT_TRUE(impl->getMessageTrace());
}
TEST(DefaultMQProducerImplTest, Sends) {
  DefaultMQProducerImpl* impl = new DefaultMQProducerImpl("testMockSendMQProducerGroup");
  MockMQClientFactory* mockFactory = new MockMQClientFactory("testClientId");
  MockMQClientAPIImpl* apiImpl = new MockMQClientAPIImpl();

  impl->setFactory(mockFactory);
  impl->setNamesrvAddr("http://rocketmq.nameserver.com");

  // prepare send
  boost::shared_ptr<TopicPublishInfo> topicPublishInfo = boost::make_shared<TopicPublishInfo>();
  MQMessageQueue mqA("TestTopic", "BrokerA", 0);
  MQMessageQueue mqB("TestTopic", "BrokerB", 0);
  topicPublishInfo->updateMessageQueueList(mqA);
  topicPublishInfo->updateMessageQueueList(mqB);

  SendResult okMQAResult(SEND_OK, "MSSAGEID", "OFFSETID", mqA, 1024, "DEFAULT_REGION");
  SendResult okMQBResult(SEND_OK, "MSSAGEID", "OFFSETID", mqB, 2048);
  okMQBResult.setRegionId("DEFAULT_REGION");
  okMQBResult.toString();
  SendResult errorMQBResult(SEND_SLAVE_NOT_AVAILABLE, "MSSAGEID", "OFFSETID", mqB, 2048);

  EXPECT_CALL(*mockFactory, start()).Times(1).WillOnce(Return());
  EXPECT_CALL(*mockFactory, shutdown()).Times(1).WillOnce(Return());
  EXPECT_CALL(*mockFactory, registerProducer(_)).Times(1).WillOnce(Return(true));
  EXPECT_CALL(*mockFactory, unregisterProducer(_)).Times(1).WillOnce(Return());
  EXPECT_CALL(*mockFactory, sendHeartbeatToAllBroker()).Times(1).WillOnce(Return());
  EXPECT_CALL(*mockFactory, tryToFindTopicPublishInfo(_, _)).WillRepeatedly(Return(topicPublishInfo));
  EXPECT_CALL(*mockFactory, findBrokerAddressInPublish(_)).WillRepeatedly(Return("BrokerA"));
  EXPECT_CALL(*mockFactory, getMQClientAPIImpl()).WillRepeatedly(Return(apiImpl));

  EXPECT_CALL(*apiImpl, sendMessage(_, _, _, _, _, _, _, _, _))
      .WillOnce(Return(okMQAResult))
      .WillOnce(Return(okMQBResult))
      .WillOnce(Return(errorMQBResult))
      .WillOnce(Return(okMQAResult))
      .WillOnce(Return(okMQAResult))
      .WillRepeatedly(Return(okMQAResult));

  // Start Producer.
  impl->start();

  MQMessage msg("testTopic", "testTag", "testKey", "testBodysA");
  SendResult s1 = impl->send(msg);
  EXPECT_EQ(s1.getSendStatus(), SEND_OK);
  EXPECT_EQ(s1.getQueueOffset(), 1024);
  SendResult s2 = impl->send(msg, mqB);
  EXPECT_EQ(s2.getSendStatus(), SEND_OK);
  EXPECT_EQ(s2.getQueueOffset(), 2048);
  MessageQueueSelector* pSelect = new MyMessageQueueSelector();
  SendResult s3 = impl->send(msg, pSelect, nullptr, 3, true);
  EXPECT_EQ(s3.getSendStatus(), SEND_OK);
  EXPECT_EQ(s3.getQueueOffset(), 1024);
  SendResult s33 = impl->send(msg, pSelect, nullptr);
  EXPECT_EQ(s33.getSendStatus(), SEND_OK);
  EXPECT_EQ(s33.getQueueOffset(), 1024);

  SendCallback* pCallback = new MySendCallback();
  EXPECT_NO_THROW(impl->send(msg, pCallback, true));
  EXPECT_NO_THROW(impl->send(msg, pSelect, nullptr, pCallback));
  EXPECT_NO_THROW(impl->send(msg, mqA, pCallback));

  EXPECT_NO_THROW(impl->sendOneway(msg));
  EXPECT_NO_THROW(impl->sendOneway(msg, mqA));
  EXPECT_NO_THROW(impl->sendOneway(msg, pSelect, nullptr));

  MQMessage msgB("testTopic", "testTag", "testKey", "testBodysB");
  vector<MQMessage> msgs;
  msgs.push_back(msg);
  msgs.push_back(msgB);
  SendResult s4 = impl->send(msgs);
  EXPECT_EQ(s4.getSendStatus(), SEND_OK);
  EXPECT_EQ(s4.getQueueOffset(), 1024);
  SendResult s5 = impl->send(msgs, mqA);
  EXPECT_EQ(s5.getSendStatus(), SEND_OK);
  EXPECT_EQ(s5.getQueueOffset(), 1024);

  impl->shutdown();
  delete mockFactory;
  delete apiImpl;
}
TEST(DefaultMQProducerImplTest, Trace) {
  DefaultMQProducerImpl* impl = new DefaultMQProducerImpl("testMockProducerTraceGroup");
  MockMQClientFactory* mockFactory = new MockMQClientFactory("testTraceClientId");
  MockMQClientAPIImpl* apiImpl = new MockMQClientAPIImpl();

  impl->setFactory(mockFactory);
  impl->setNamesrvAddr("http://rocketmq.nameserver.com");
  impl->setMessageTrace(true);

  // prepare send
  boost::shared_ptr<TopicPublishInfo> topicPublishInfo = boost::make_shared<TopicPublishInfo>();
  MQMessageQueue mqA("TestTraceTopic", "BrokerA", 0);
  MQMessageQueue mqB("TestTraceTopic", "BrokerB", 0);
  topicPublishInfo->updateMessageQueueList(mqA);
  topicPublishInfo->updateMessageQueueList(mqB);

  SendResult okMQAResult(SEND_OK, "MSSAGEID", "OFFSETID", mqA, 1024, "DEFAULT_REGION");

  EXPECT_CALL(*mockFactory, start()).Times(1).WillOnce(Return());
  EXPECT_CALL(*mockFactory, shutdown()).Times(1).WillOnce(Return());
  EXPECT_CALL(*mockFactory, registerProducer(_)).Times(1).WillOnce(Return(true));
  EXPECT_CALL(*mockFactory, unregisterProducer(_)).Times(1).WillOnce(Return());
  EXPECT_CALL(*mockFactory, sendHeartbeatToAllBroker()).Times(1).WillOnce(Return());
  EXPECT_CALL(*mockFactory, tryToFindTopicPublishInfo(_, _)).WillRepeatedly(Return(topicPublishInfo));
  EXPECT_CALL(*mockFactory, findBrokerAddressInPublish(_)).WillRepeatedly(Return("BrokerA"));
  EXPECT_CALL(*mockFactory, getMQClientAPIImpl()).WillRepeatedly(Return(apiImpl));

  EXPECT_CALL(*apiImpl, sendMessage(_, _, _, _, _, _, _, _, _)).WillRepeatedly(Return(okMQAResult));

  // Start Producer.
  impl->start();

  MQMessage msg("TestTraceTopic", "testTag", "testKey", "testBodysA");
  SendResult s1 = impl->send(msg);
  EXPECT_EQ(s1.getSendStatus(), SEND_OK);

  impl->shutdown();
  delete mockFactory;
  delete apiImpl;
}
int main(int argc, char* argv[]) {
  InitGoogleMock(&argc, argv);
  return RUN_ALL_TESTS();
}
