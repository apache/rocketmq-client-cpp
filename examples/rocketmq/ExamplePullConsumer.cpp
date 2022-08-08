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
#include <cstdlib>
#include <iostream>

#include "rocketmq/CredentialsProvider.h"
#include "rocketmq/DefaultMQPullConsumer.h"
#include "rocketmq/Logger.h"

int main(int argc, char* argv[]) {
  const char* group = "GID_group003";
  const char* topic = "yc001";
  const char* resource_namespace = "MQ_INST_1973281269661160_BXmPlOA6";
  const char* name_server_list = "ipv4:11.165.223.199:9876";

  rocketmq::Logger& logger = rocketmq::getLogger();
  logger.setLevel(rocketmq::Level::Debug);
  logger.init();

  rocketmq::DefaultMQPullConsumer pull_consumer(group);
  pull_consumer.setResourceNamespace(resource_namespace);
  pull_consumer.setCredentialsProvider(std::make_shared<rocketmq::ConfigFileCredentialsProvider>());
  pull_consumer.setNamesrvAddr(name_server_list);
  pull_consumer.start();

  std::future<std::vector<rocketmq::MQMessageQueue>> future = pull_consumer.queuesFor(topic);
  auto queues = future.get();

  for (const auto& queue : queues) {
    rocketmq::OffsetQuery offset_query;
    offset_query.message_queue = queue;
    offset_query.policy = rocketmq::QueryOffsetPolicy::BEGINNING;
    auto offset_future = pull_consumer.queryOffset(offset_query);
    int64_t offset = offset_future.get();
    std::cout << "offset: " << offset << std::endl;
  }

  pull_consumer.shutdown();
  return EXIT_SUCCESS;
}