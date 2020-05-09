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

#include "../src/common/UtilAll.h"
#include "../src/log/Logging.h"
#include "MessageUtil.h"
#include "common.h"

using namespace rocketmq;

class MyResponseMessageListener : public MessageListenerConcurrently {
 public:
  MyResponseMessageListener(DefaultMQProducer* replyProducer) : m_replyProducer(replyProducer) {}
  virtual ~MyResponseMessageListener() = default;

  ConsumeStatus consumeMessage(const std::vector<MQMessageExt*>& msgs) override {
    for (const auto& msg : msgs) {
      try {
        std::cout << "handle message: " << msg->toString() << std::endl;
        const auto& replyTo = MessageUtil::getReplyToClient(msg);

        // create reply message with given util, do not create reply message by yourself
        std::unique_ptr<MQMessage> replyMessage(MessageUtil::createReplyMessage(msg, "reply message contents."));

        // send reply message with producer
        SendResult replyResult = m_replyProducer->send(replyMessage.get(), 10000);
        std::cout << "reply to " << replyTo << ", " << replyResult.toString() << std::endl;
      } catch (const std::exception& e) {
        std::cout << e.what() << std::endl;
      }
    }
    return CONSUME_SUCCESS;
  }

 private:
  DefaultMQProducer* m_replyProducer;
};

int main(int argc, char* argv[]) {
  RocketmqSendAndConsumerArgs info;
  if (!ParseArgs(argc, argv, &info)) {
    exit(-1);
  }
  PrintRocketmqSendAndConsumerArgs(info);

  DefaultMQProducer producer("");
  producer.setNamesrvAddr(info.namesrv);
  producer.setSendMsgTimeout(3000);
  producer.setRetryTimes(info.retrytimes);
  producer.setRetryTimes4Async(info.retrytimes);
  producer.setSendLatencyFaultEnable(!info.selectUnactiveBroker);
  producer.setTcpTransportTryLockTimeout(1000);
  producer.setTcpTransportConnectTimeout(400);
  producer.start();

  DefaultMQPushConsumer consumer(info.groupname);
  consumer.setNamesrvAddr(info.namesrv);
  consumer.setTcpTransportTryLockTimeout(1000);
  consumer.setTcpTransportConnectTimeout(400);
  consumer.setConsumeThreadNum(info.thread_count);
  consumer.setConsumeFromWhere(CONSUME_FROM_LAST_OFFSET);

  // recommend client configs
  consumer.setPullTimeDelayMillsWhenException(0L);

  consumer.subscribe(info.topic, "*");

  MyResponseMessageListener msglistener(&producer);
  consumer.registerMessageListener(&msglistener);

  try {
    consumer.start();
  } catch (MQClientException& e) {
    std::cout << e << std::endl;
    return -1;
  }

  // std::this_thread::sleep_for(std::chrono::seconds(10));

  int msg_count = g_msgCount.load();
  for (int count = 0; count < msg_count; count++) {
    try {
      MQMessage msg(info.topic, "Hello world");

      auto begin = UtilAll::currentTimeMillis();
      std::unique_ptr<MQMessage> retMsg(producer.request(&msg, 10000));
      auto cost = UtilAll::currentTimeMillis() - begin;
      std::cout << count << " >>> request to <" << info.topic << "> cost: " << cost
                << " replyMessage: " << retMsg->toString() << std::endl;
    } catch (const std::exception& e) {
      std::cout << count << " >>> " << e.what() << std::endl;
    }
  }

  consumer.shutdown();
  producer.shutdown();

  return 0;
}
