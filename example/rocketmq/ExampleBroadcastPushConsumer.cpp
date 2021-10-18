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
#include "rocketmq/DefaultMQPushConsumer.h"

#include "rocketmq/Logger.h"
#include "spdlog/spdlog.h"

#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>

using namespace rocketmq;

class SampleMQMessageListener : public StandardMessageListener {
public:
  ConsumeMessageResult consumeMessage(const std::vector<MQMessageExt>& msgs) override {
    for (const MQMessageExt& msg : msgs) {
      SPDLOG_INFO("Receive a message. MessageId={}", msg.getMsgId());
      std::cout << "Received a message. MessageId: " << msg.getMsgId() << std::endl;
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return ConsumeMessageResult::SUCCESS;
  }
};

int main(int argc, char* argv[]) {
  Logger& logger = getLogger();
  logger.setLevel(Level::Debug);
  logger.init();

  const char* cid = "GID_cpp_sdk_standard";
  const char* topic = "cpp_sdk_standard";
  const char* resource_namespace = "MQ_INST_1080056302921134_BXuIbML7";

  DefaultMQPushConsumer push_consumer(cid);
  push_consumer.setMessageModel(MessageModel::BROADCASTING);
  push_consumer.setResourceNamespace(resource_namespace);
  push_consumer.setCredentialsProvider(std::make_shared<ConfigFileCredentialsProvider>());
  push_consumer.setNamesrvAddr("121.43.42.193:80");
  MessageListener* listener = new SampleMQMessageListener;
  push_consumer.setGroupName(cid);
  push_consumer.subscribe(topic, "*");
  push_consumer.registerMessageListener(listener);
  push_consumer.start();

  std::this_thread::sleep_for(std::chrono::minutes(60));

  push_consumer.shutdown();
  return EXIT_SUCCESS;
}
