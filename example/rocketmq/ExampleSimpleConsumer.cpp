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
#include <cstdlib>
#include <iostream>
#include <system_error>
#include <thread>

#include "rocketmq/Logger.h"
#include "rocketmq/RocketMQ.h"
#include "rocketmq/SimpleConsumer.h"

using namespace ROCKETMQ_NAMESPACE;

int main(int argc, char* argv[]) {

  Logger& logger = getLogger();
  logger.setLevel(Level::Debug);
  logger.init();

  const char* group_id = "GID_cpp_sdk_standard";
  const char* topic = "cpp_sdk_standard";
  const char* resource_namespace = "MQ_INST_1080056302921134_BXuIbML7";
  const char* name_server = "mq-inst-1080056302921134-bxuibml7.mq.cn-hangzhou.aliyuncs.com:80";

  SimpleConsumer simple_consumer(group_id);
  simple_consumer.setResourceNamespace(resource_namespace);
  simple_consumer.setCredentialsProvider(std::make_shared<ConfigFileCredentialsProvider>());
  simple_consumer.setNamesrvAddr(name_server);
  simple_consumer.setInstanceName("instance_0");
  simple_consumer.subscribe(topic, "*");
  simple_consumer.start();

  std::error_code ec;
  auto&& messages = simple_consumer.receive(topic, std::chrono::seconds(10), ec);
  if (ec) {
    std::cerr << "Failed to receive message. Cause: " << ec.message() << std::endl;
  } else {
    for (const auto& message : messages) {
      std::cout << "Message-ID: " << message.getMsgId() << std::endl;
    }
  }

  return EXIT_SUCCESS;
}