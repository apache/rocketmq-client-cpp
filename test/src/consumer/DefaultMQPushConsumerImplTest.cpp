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
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "ConsumeMessageContext.h"
#include "ConsumeMessageHookImpl.h"
#include "DefaultMQProducerImpl.h"
#include "DefaultMQPushConsumerImpl.h"
#include "MQMessageExt.h"
#include "MQMessageQueue.h"

using namespace std;
using namespace rocketmq;
using rocketmq::DefaultMQProducerImpl;
using rocketmq::DefaultMQPushConsumerImpl;
using testing::_;
using ::testing::InitGoogleMock;
using ::testing::InitGoogleTest;
using testing::Return;

class MockDefaultMQProducerImpl : public DefaultMQProducerImpl {
 public:
  MockDefaultMQProducerImpl(const string& groupId) : DefaultMQProducerImpl(groupId) {}
  MOCK_METHOD3(send, void(MQMessage&, SendCallback*, bool));
};
TEST(DefaultMQPushConsumerImplTest, init) {
  DefaultMQPushConsumerImpl* impl = new DefaultMQPushConsumerImpl("testMQConsumerGroup");
  EXPECT_EQ(impl->getGroupName(), "testMQConsumerGroup");
  impl->setUnitName("testUnit");
  EXPECT_EQ(impl->getUnitName(), "testUnit");
  impl->setTcpTransportPullThreadNum(64);
  EXPECT_EQ(impl->getTcpTransportPullThreadNum(), 64);
  impl->setTcpTransportConnectTimeout(2000);
  EXPECT_EQ(impl->getTcpTransportConnectTimeout(), 2000);
  impl->setTcpTransportTryLockTimeout(3000);
  EXPECT_EQ(impl->getTcpTransportTryLockTimeout(), 3);
  impl->setNamesrvAddr("http://rocketmq.nameserver.com");
  EXPECT_EQ(impl->getNamesrvAddr(), "rocketmq.nameserver.com");
  impl->setNameSpace("MQ_INST_NAMESPACE_TEST");
  EXPECT_EQ(impl->getNameSpace(), "MQ_INST_NAMESPACE_TEST");
  impl->setMessageTrace(true);
  EXPECT_TRUE(impl->getMessageTrace());
  impl->setAsyncPull(true);

  impl->setConsumeMessageBatchMaxSize(3000);
  EXPECT_EQ(impl->getConsumeMessageBatchMaxSize(), 3000);

  impl->setConsumeThreadCount(3);
  EXPECT_EQ(impl->getConsumeThreadCount(), 3);

  impl->setMaxReconsumeTimes(30);
  EXPECT_EQ(impl->getMaxReconsumeTimes(), 30);

  impl->setMaxCacheMsgSizePerQueue(3000);
  EXPECT_EQ(impl->getMaxCacheMsgSizePerQueue(), 3000);

  impl->setPullMsgThreadPoolCount(10);
  EXPECT_EQ(impl->getPullMsgThreadPoolCount(), 10);
}

TEST(DefaultMQPushConsumerImpl, Trace) {
  DefaultMQPushConsumerImpl* impl = new DefaultMQPushConsumerImpl();
  MockDefaultMQProducerImpl* implProducer = new MockDefaultMQProducerImpl("testMockProducerTraceGroup");
  std::shared_ptr<ConsumeMessageHook> hook(new ConsumeMessageHookImpl());
  impl->setMessageTrace(true);
  impl->setDefaultMqProducerImpl(implProducer);
  impl->registerConsumeMessageHook(hook);
  EXPECT_CALL(*implProducer, send(_, _, _)).WillRepeatedly(Return());

  ConsumeMessageContext consumeMessageContext;
  MQMessageQueue messageQueue("TestTopic", "BrokerA", 0);
  MQMessageExt messageExt;
  messageExt.setMsgId("MessageID");
  messageExt.setKeys("MessageKey");
  vector<MQMessageExt> msgs;
  consumeMessageContext.setDefaultMQPushConsumer(impl);
  consumeMessageContext.setConsumerGroup("testMockProducerTraceGroup");
  consumeMessageContext.setMessageQueue(messageQueue);
  consumeMessageContext.setMsgList(msgs);
  consumeMessageContext.setSuccess(false);
  consumeMessageContext.setNameSpace("NameSpace");
  impl->executeConsumeMessageHookBefore(&consumeMessageContext);
  impl->executeConsumeMessageHookAfter(&consumeMessageContext);

  msgs.push_back(messageExt);
  consumeMessageContext.setMsgList(msgs);

  impl->executeConsumeMessageHookBefore(&consumeMessageContext);

  consumeMessageContext.setMsgIndex(0);
  consumeMessageContext.setStatus("CONSUME_SUCCESS");
  consumeMessageContext.setSuccess(true);
  impl->executeConsumeMessageHookAfter(&consumeMessageContext);
  EXPECT_TRUE(impl->hasConsumeMessageHook());
  delete implProducer;
}
int main(int argc, char* argv[]) {
  InitGoogleMock(&argc, argv);
  return RUN_ALL_TESTS();
}
