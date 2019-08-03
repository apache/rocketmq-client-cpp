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

using namespace rocketmq;

TpsReportService g_tps;
latch g_finished(1);

class MyMsgListener : public MessageListenerOrderly {
 public:
  MyMsgListener() {}
  virtual ~MyMsgListener() {}

  virtual ConsumeStatus consumeMessage(const std::vector<MQMessageExt*>& msgs) {
    auto old = g_msgCount.fetch_sub(msgs.size());
    if (old > 0) {
      for (size_t i = 0; i < msgs.size(); ++i) {
        g_tps.Increment();
        // std::cout << msgs[i]->getMsgId() << std::endl;
        // std::cout << "msg body: " << msgs[i].getBody() << std::endl;
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

  DefaultMQPushConsumer consumer("please_rename_unique_group_name");
  consumer.setNamesrvAddr(info.namesrv);
  consumer.setGroupName(info.groupname);
  consumer.setTcpTransportTryLockTimeout(1000);
  consumer.setTcpTransportConnectTimeout(400);
  consumer.setConsumeThreadCount(info.thread_count);
  consumer.setConsumeMessageBatchMaxSize(31);
  consumer.setConsumeFromWhere(CONSUME_FROM_LAST_OFFSET);
  consumer.subscribe(info.topic, "*");

  MyMsgListener msglistener;
  consumer.registerMessageListener(&msglistener);

  g_tps.start();

  try {
    consumer.start();
  } catch (MQClientException& e) {
    std::cout << e << std::endl;
  }

  g_finished.wait();

  consumer.shutdown();

  return 0;
}
