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
#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>

#include "rocketmq/Logger.h"

#include "spdlog/spdlog.h"

#include "rocketmq/DefaultMQPushConsumer.h"

    using namespace rocketmq;

class SampleMQMessageListener : public StandardMessageListener {
public:
  ConsumeMessageResult consumeMessage(const std::vector<MQMessageExt>& msgs) override {
    for (const MQMessageExt& msg : msgs) {
      SPDLOG_INFO("Consume message[Topic={}, MessageId={}] OK", msg.getTopic(), msg.getMsgId());
      std::cout << "Consume Message[MsgId=" << msg.getMsgId() << "] OK. Body Size: " << msg.getBody().size()
                << std::endl;
      // std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    return ConsumeMessageResult::SUCCESS;
  }
};

int main(int argc, char* argv[]) {

  Logger& logger = getLogger();
  logger.setLevel(Level::Debug);
  logger.init();

  const char* group_id = "GID_cpp_sdk_standard";
  const char* topic = "cpp_sdk_standard";
  const char* resource_namespace = "MQ_INST_1080056302921134_BXuIbML7";

  DefaultMQPushConsumer push_consumer(group_id);
  push_consumer.setResourceNamespace(resource_namespace);
  push_consumer.setCredentialsProvider(std::make_shared<ConfigFileCredentialsProvider>());
  push_consumer.setNamesrvAddr("121.43.42.193:80");
  MessageListener* listener = new SampleMQMessageListener;
  push_consumer.setInstanceName("instance_0");
  push_consumer.subscribe(topic, "*");
  push_consumer.registerMessageListener(listener);
  push_consumer.setConsumeThreadCount(4);
  push_consumer.start();

  std::this_thread::sleep_for(std::chrono::minutes(30));

  push_consumer.shutdown();
  return EXIT_SUCCESS;
}
