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

#include <atomic>
#include <condition_variable>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <thread>
#include "TransactionListener.h"
#include "TransactionMQProducer.h"
#include "TransactionSendResult.h"
#include "common.h"

using namespace rocketmq;

std::atomic<bool> g_quit;
std::mutex g_mtx;
std::condition_variable g_finished;
TpsReportService g_tps;

class MyTransactionListener : public TransactionListener {
  virtual LocalTransactionState executeLocalTransaction(const MQMessage& msg, void* arg) {
    if (!arg) {
      std::cout << "executeLocalTransaction transactionId:" << msg.getTransactionId()
                << ", return state: COMMIT_MESAGE " << endl;
      return LocalTransactionState::COMMIT_MESSAGE;
    }

    LocalTransactionState state = (LocalTransactionState)(*(int*)arg % 3);
    std::cout << "executeLocalTransaction transactionId:" << msg.getTransactionId() << ", return state: " << state
              << endl;
    return state;
  }

  virtual LocalTransactionState checkLocalTransaction(const MQMessageExt& msg) {
    std::cout << "checkLocalTransaction enter msg:" << msg.toString() << endl;
    return LocalTransactionState::COMMIT_MESSAGE;
  }
};

void SyncProducerWorker(RocketmqSendAndConsumerArgs* info, TransactionMQProducer* producer) {
  while (!g_quit.load()) {
    if (g_msgCount.load() <= 0) {
      std::this_thread::sleep_for(std::chrono::seconds(60));
      std::unique_lock<std::mutex> lck(g_mtx);
      g_finished.notify_one();
      break;
    }

    MQMessage msg(info->topic,  // topic
                  "*",          // tag
                  info->body);  // body
    try {
      auto start = std::chrono::system_clock::now();
      std::cout << "before sendMessageInTransaction" << endl;
      LocalTransactionState state = LocalTransactionState::UNKNOWN;
      TransactionSendResult sendResult = producer->sendMessageInTransaction(msg, &state);
      std::cout << "after sendMessageInTransaction msgId: " << sendResult.getMsgId() << endl;
      g_tps.Increment();
      --g_msgCount;
      auto end = std::chrono::system_clock::now();
      auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
      if (duration.count() >= 500) {
        std::cout << "send RT more than: " << duration.count() << " ms with msgid: " << sendResult.getMsgId() << endl;
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
  TransactionMQProducer producer("please_rename_unique_group_name");
  producer.setNamesrvAddr(info.namesrv);
  producer.setNamesrvDomain(info.namesrv_domain);
  producer.setGroupName(info.groupname);
  producer.setInstanceName(info.groupname);
  producer.setSessionCredentials("mq acesskey", "mq secretkey", "ALIYUN");
  producer.setSendMsgTimeout(500);
  producer.setTcpTransportTryLockTimeout(1000);
  producer.setTcpTransportConnectTimeout(400);
  producer.setLogLevel(eLOG_LEVEL_DEBUG);
  producer.setTransactionListener(new MyTransactionListener());
  producer.start();
  std::vector<std::shared_ptr<std::thread>> work_pool;
  auto start = std::chrono::system_clock::now();
  int msgcount = g_msgCount.load();
  g_tps.start();

  int threadCount = info.thread_count;
  for (int j = 0; j < threadCount; j++) {
    std::shared_ptr<std::thread> th = std::make_shared<std::thread>(SyncProducerWorker, &info, &producer);
    work_pool.push_back(th);
  }

  {
    std::unique_lock<std::mutex> lck(g_mtx);
    g_finished.wait(lck);
    g_quit.store(true);
  }

  auto end = std::chrono::system_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

  std::cout << "per msg time: " << duration.count() / (double)msgcount << "ms \n"
            << "========================finished==============================\n";

  for (size_t th = 0; th != work_pool.size(); ++th) {
    work_pool[th]->join();
  }

  producer.shutdown();

  return 0;
}
