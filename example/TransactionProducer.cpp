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
#include "TransactionMQProducer.h"

using namespace rocketmq;

TpsReportService g_tps;

class MyTransactionListener : public TransactionListener {
  virtual LocalTransactionState executeLocalTransaction(const MQMessage& msg, void* arg) {
    LocalTransactionState state = (LocalTransactionState)(((intptr_t)arg) % 3);
    std::cout << "executeLocalTransaction transactionId:" << msg.transaction_id() << ", return state: " << state
              << std::endl;
    return state;
  }

  virtual LocalTransactionState checkLocalTransaction(const MQMessageExt& msg) {
    std::cout << "checkLocalTransaction enter msg:" << msg.toString() << std::endl;
    return LocalTransactionState::COMMIT_MESSAGE;
  }
};

void SyncProducerWorker(RocketmqSendAndConsumerArgs* info, TransactionMQProducer* producer) {
  int old = g_msg_count.fetch_sub(1);
  while (old > 0) {
    MQMessage msg(info->topic,  // topic
                  "*",          // tag
                  info->body);  // body
    try {
      auto start = std::chrono::system_clock::now();
      intptr_t arg = old - 1;
      TransactionSendResult sendResult = producer->sendMessageInTransaction(msg, (void*)arg);
      auto end = std::chrono::system_clock::now();

      g_tps.Increment();

      auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
      if (duration.count() >= 500) {
        std::cout << "send RT more than: " << duration.count() << "ms with msgid: " << sendResult.msg_id() << std::endl;
      }
    } catch (const MQException& e) {
      std::cout << "send failed: " << e.what() << std::endl;
    }
    old = g_msg_count.fetch_sub(1);
  }
}

int main(int argc, char* argv[]) {
  RocketmqSendAndConsumerArgs info;
  if (!ParseArgs(argc, argv, &info)) {
    exit(-1);
  }
  PrintRocketmqSendAndConsumerArgs(info);

  auto* producer = new TransactionMQProducer(info.groupname);
  producer->set_namesrv_addr(info.namesrv);
  producer->set_group_name(info.groupname);
  producer->set_send_msg_timeout(3000);
  producer->set_retry_times(info.retrytimes);
  producer->set_retry_times_for_async(info.retrytimes);
  producer->set_send_latency_fault_enable(!info.selectUnactiveBroker);
  producer->set_tcp_transport_try_lock_timeout(1000);
  producer->set_tcp_transport_connect_timeout(400);

  MyTransactionListener myListener;
  producer->setTransactionListener(&myListener);

  producer->start();

  std::vector<std::shared_ptr<std::thread>> work_pool;
  int msgcount = g_msg_count.load();
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

  std::this_thread::sleep_for(std::chrono::seconds(30));
  producer->shutdown();

  delete producer;

  return 0;
}
