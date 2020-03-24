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

#include <map>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "ConsumerRunningInfo.h"
#include "DefaultMQPushConsumerImpl.h"
#include "MQClientFactory.h"

using namespace std;
using namespace rocketmq;
using rocketmq::ConsumerRunningInfo;
using rocketmq::DefaultMQPushConsumerImpl;
using rocketmq::MQClientFactory;
using rocketmq::TopicRouteData;
using testing::_;
using ::testing::InitGoogleMock;
using ::testing::InitGoogleTest;
using testing::Mock;
using testing::Return;

class MockPushConsumerImpl : public DefaultMQPushConsumerImpl {
 public:
  MockPushConsumerImpl(const std::string& groupname) : DefaultMQPushConsumerImpl() {}
  MOCK_METHOD0(getConsumerRunningInfo, ConsumerRunningInfo*());
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

  MOCK_METHOD5(getMinOffset, int64(const string&, const string&, int, int, const SessionCredentials&));
  MOCK_METHOD3(getTopicRouteInfoFromNameServer, TopicRouteData*(const string&, int, const SessionCredentials&));
};
class MockMQClientFactory : public MQClientFactory {
 public:
  MockMQClientFactory(const string& mqClientId,
                      int pullThreadNum,
                      uint64_t tcpConnectTimeout,
                      uint64_t tcpTransportTryLockTimeout,
                      string unitName)
      : MQClientFactory(mqClientId, true, DEFAULT_SSL_PROPERTY_FILE) {}
  void reInitClientImpl(MQClientAPIImpl* pImpl) { m_pClientAPIImpl.reset(pImpl); }
  void addTestConsumer(const string& consumerName, MQConsumer* pMQConsumer) {
    addConsumerToTable(consumerName, pMQConsumer);
  }
};

TEST(MQClientFactoryTest, minOffset) {
  string clientId = "testClientId";
  int pullThreadNum = 1;
  uint64_t tcpConnectTimeout = 3000;
  uint64_t tcpTransportTryLockTimeout = 3000;
  string unitName = "central";
  MockMQClientFactory* factory =
      new MockMQClientFactory(clientId, pullThreadNum, tcpConnectTimeout, tcpTransportTryLockTimeout, unitName);
  MockMQClientAPIImpl* pImpl = new MockMQClientAPIImpl(clientId, nullptr, pullThreadNum, tcpConnectTimeout,
                                                       tcpTransportTryLockTimeout, unitName);
  factory->reInitClientImpl(pImpl);
  MQMessageQueue mq;
  mq.setTopic("testTopic");
  mq.setBrokerName("testBroker");
  mq.setQueueId(1);
  SessionCredentials session_credentials;

  TopicRouteData* pData = new TopicRouteData();
  pData->setOrderTopicConf("OrderTopicConf");
  QueueData qd;
  qd.brokerName = "testBroker";
  qd.readQueueNums = 8;
  qd.writeQueueNums = 8;
  qd.perm = 1;
  pData->getQueueDatas().push_back(qd);
  BrokerData bd;
  bd.brokerName = "testBroker";
  bd.brokerAddrs[0] = "127.0.0.1:10091";
  bd.brokerAddrs[1] = "127.0.0.2:10092";
  pData->getBrokerDatas().push_back(bd);

  EXPECT_CALL(*pImpl, getMinOffset(_, _, _, _, _)).Times(1).WillOnce(Return(1024));
  EXPECT_CALL(*pImpl, getTopicRouteInfoFromNameServer(_, _, _)).Times(1).WillOnce(Return(pData));
  int64 offset = factory->minOffset(mq, session_credentials);
  EXPECT_EQ(1024, offset);
  delete factory;
}

TEST(MQClientFactoryTest, consumerRunningInfo) {
  string clientId = "testClientId";
  int pullThreadNum = 1;
  uint64_t tcpConnectTimeout = 3000;
  uint64_t tcpTransportTryLockTimeout = 3000;
  string unitName = "central";
  MockMQClientFactory* factory =
      new MockMQClientFactory(clientId, pullThreadNum, tcpConnectTimeout, tcpTransportTryLockTimeout, unitName);
  MockPushConsumerImpl* mockPushConsumer = new MockPushConsumerImpl(clientId);
  Mock::AllowLeak(mockPushConsumer);
  factory->addTestConsumer(clientId, mockPushConsumer);
  ConsumerRunningInfo* info = new ConsumerRunningInfo();
  info->setJstack("Hello,JStack");
  EXPECT_CALL(*mockPushConsumer, getConsumerRunningInfo()).Times(1).WillOnce(Return(info));
  ConsumerRunningInfo* info2 = factory->consumerRunningInfo(clientId);
  EXPECT_EQ(info2->getJstack(), "Hello,JStack");
  delete factory;
}

int main(int argc, char* argv[]) {
  InitGoogleMock(&argc, argv);
  return RUN_ALL_TESTS();
}
