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
#include "common.h"
#include "concurrent/latch.hpp"
#include "DefaultMQPushConsumer.h"
#include "UtilAll.h"

using namespace rocketmq;

TpsReportService g_tps;
latch g_finished(1);

class MyMsgListener : public MessageListenerConcurrently {
 public:
  virtual ~MyMsgListener() = default;

  ConsumeStatus consumeMessage(std::vector<MQMessageExt>& msgs) override {
    auto old = g_msg_count.fetch_sub(msgs.size());
    if (old > 0) {
      for (size_t i = 0; i < msgs.size(); ++i) {
        g_tps.Increment();
        if (msgs[i] == nullptr) {
          std::cout << "error!!!" << std::endl;
        } else {
          std::cout << msgs[i].msg_id() << ", body: " << msgs[i].body() << std::endl;
        }
      }
      if (old <= msgs.size()) {
        g_finished.count_down();
      }
      return CONSUME_SUCCESS;
    } else {
      return RECONSUME_LATER;
    }
  }
};

int main(int argc, char* argv[]) {
  RocketmqSendAndConsumerArgs info;
  if (!ParseArgs(argc, argv, &info)) {
    exit(-1);
  }
  PrintRocketmqSendAndConsumerArgs(info);

  auto* consumer = new DefaultMQPushConsumer(info.groupname);
  consumer->set_namesrv_addr(info.namesrv);
  consumer->set_group_name(info.groupname);
  consumer->set_tcp_transport_try_lock_timeout(1000);
  consumer->set_tcp_transport_connect_timeout(400);
  consumer->set_consume_thread_nums(info.thread_count);
  consumer->set_consume_from_where(CONSUME_FROM_LAST_OFFSET);

  if (info.broadcasting) {
    consumer->set_message_model(BROADCASTING);
  }

  consumer->subscribe(info.topic, "*");

  MyMsgListener msglistener;
  consumer->registerMessageListener(&msglistener);

  g_tps.start();

  try {
    consumer->start();
  } catch (MQClientException& e) {
    std::cout << e << std::endl;
  }

  g_finished.wait();

  consumer->shutdown();

  delete consumer;

  return 0;
}
