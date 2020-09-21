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
#include "common.h"
#include "MessageUtil.h"
#include "DefaultMQProducer.h"
#include "DefaultMQPushConsumer.h"

using namespace rocketmq;

class MyResponseMessageListener : public MessageListenerConcurrently {
 public:
  MyResponseMessageListener(DefaultMQProducer* replyProducer) : m_replyProducer(replyProducer) {}
  virtual ~MyResponseMessageListener() = default;

  ConsumeStatus consumeMessage(std::vector<MQMessageExt>& msgs) override {
    for (const auto& msg : msgs) {
      try {
        std::cout << "handle message: " << msg.toString() << std::endl;
        const auto& replyTo = MessageUtil::getReplyToClient(msg);

        // create reply message with given util, do not create reply message by yourself
        MQMessage replyMessage = MessageUtil::createReplyMessage(msg, "reply message contents.");

        // send reply message with producer
        SendResult replyResult = m_replyProducer->send(replyMessage, 10000);
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
  producer.set_namesrv_addr(info.namesrv);
  producer.set_send_msg_timeout(3000);
  producer.set_retry_times(info.retrytimes);
  producer.set_retry_times_for_async(info.retrytimes);
  producer.set_send_latency_fault_enable(!info.selectUnactiveBroker);
  producer.set_tcp_transport_try_lock_timeout(1000);
  producer.set_tcp_transport_connect_timeout(400);
  producer.start();

  DefaultMQPushConsumer consumer(info.groupname);
  consumer.set_namesrv_addr(info.namesrv);
  consumer.set_tcp_transport_try_lock_timeout(1000);
  consumer.set_tcp_transport_connect_timeout(400);
  consumer.set_consume_thread_nums(info.thread_count);
  consumer.set_consume_from_where(CONSUME_FROM_LAST_OFFSET);

  // recommend client configs
  consumer.set_pull_time_delay_millis_when_exception(0L);

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

  int msg_count = g_msg_count.load();
  for (int count = 0; count < msg_count; count++) {
    try {
      MQMessage msg(info.topic, "Hello world");

      auto begin = UtilAll::currentTimeMillis();
      MQMessage retMsg = producer.request(msg, 10000);
      auto cost = UtilAll::currentTimeMillis() - begin;
      std::cout << count << " >>> request to <" << info.topic << "> cost: " << cost
                << " replyMessage: " << retMsg.toString() << std::endl;
    } catch (const std::exception& e) {
      std::cout << count << " >>> " << e.what() << std::endl;
    }
  }

  consumer.shutdown();
  producer.shutdown();

  return 0;
}
