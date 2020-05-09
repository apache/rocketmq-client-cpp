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

using namespace rocketmq;

TpsReportService g_tps;

void SyncProducerWorker(RocketmqSendAndConsumerArgs* info, DefaultMQProducer* producer) {
  while (g_msgCount.fetch_sub(1) > 0) {
    MQMessage msg(info->topic,  // topic
                  "*",          // tag
                  info->body);  // body
    try {
      auto start = std::chrono::system_clock::now();
      SendResult sendResult = producer->send(&msg);
      auto end = std::chrono::system_clock::now();

      g_tps.Increment();

      auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
      if (duration.count() >= 500) {
        std::cout << "send RT more than: " << duration.count() << "ms with msgid: " << sendResult.getMsgId()
                  << std::endl;
      }
    } catch (const MQException& e) {
      std::cout << "send failed: " << e.what() << std::endl;
    }
  }
}

int main(int argc, char* argv[]) {
  RocketmqSendAndConsumerArgs info;
  if (!ParseArgs(argc, argv, &info)) {
    exit(-1);
  }
  PrintRocketmqSendAndConsumerArgs(info);

  auto* producer = new DefaultMQProducer(info.groupname);
  producer->setNamesrvAddr(info.namesrv);
  producer->setGroupName(info.groupname);
  producer->setSendMsgTimeout(3000);
  producer->setRetryTimes(info.retrytimes);
  producer->setRetryTimes4Async(info.retrytimes);
  producer->setSendLatencyFaultEnable(!info.selectUnactiveBroker);
  producer->setTcpTransportTryLockTimeout(1000);
  producer->setTcpTransportConnectTimeout(400);
  producer->start();

  std::vector<std::shared_ptr<std::thread>> work_pool;
  int msgcount = g_msgCount.load();
  g_tps.start();

  auto start = std::chrono::system_clock::now();

  int threadCount = info.thread_count;
  for (int j = 0; j < threadCount; j++) {
    auto th = std::make_shared<std::thread>(SyncProducerWorker, &info, producer);
    work_pool.push_back(th);
  }

  for (size_t th = 0; th != work_pool.size(); ++th) {
    work_pool[th]->join();
  }

  auto end = std::chrono::system_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

  std::cout << "per msg time: " << duration.count() / (double)msgcount << "ms" << std::endl
            << "========================finished=============================" << std::endl;

  producer->shutdown();

  delete producer;

  return 0;
}
