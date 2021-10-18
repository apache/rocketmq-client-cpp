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
 */#include "rocketmq/DefaultMQProducer.h"
#include <algorithm>
#include <atomic>
#include <iostream>
#include <random>

using namespace rocketmq;

const std::string& alphaNumeric() {
  static std::string alpha_numeric("0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");
  return alpha_numeric;
}

std::string randomString(std::string::size_type len) {
  std::string result;
  result.reserve(len);
  std::random_device rd;
  std::mt19937 generator(rd());
  std::string source(alphaNumeric());
  std::string::size_type generated = 0;
  while (generated < len) {
    std::shuffle(source.begin(), source.end(), generator);
    std::string::size_type delta = std::min({len - generated, source.length()});
    result.append(source.substr(0, delta));
    generated += delta;
  }
  return result;
}

int main(int argc, char* argv[]) {
  Logger& logger = getLogger();
  logger.setLevel(Level::Debug);
  logger.init();

  DefaultMQProducer producer("TestGroup");

  const char* topic = "cpp_sdk_standard";
  const char* name_server = "121.43.42.193:80";

  producer.setNamesrvAddr(name_server);
  producer.compressBodyThreshold(256);
  const char* resource_namespace = "MQ_INST_1080056302921134_BXuIbML7";
  producer.setRegion("cn-hangzhou-pre");
  producer.setResourceNamespace(resource_namespace);
  producer.setCredentialsProvider(std::make_shared<ConfigFileCredentialsProvider>());

  MQMessage message;
  message.setTopic(topic);
  message.setTags("TagA");
  message.setKey("Yuck! Why-plural?");

  std::atomic_bool stopped;
  std::atomic_long count(0);

  auto stats_lambda = [&] {
    while (!stopped.load(std::memory_order_relaxed)) {
      long cnt = count.load(std::memory_order_relaxed);
      while (count.compare_exchange_weak(cnt, 0)) {
        break;
      }
      std::this_thread::sleep_for(std::chrono::seconds(1));
      std::cout << "QPS: " << cnt << std::endl;
    }
  };

  std::thread stats_thread(stats_lambda);

  std::string body = randomString(1024 * 4);
  std::cout << "Message body size: " << body.length() << std::endl;
  message.setBody(body);

  try {
    producer.start();
    for (int i = 0; i < 16; ++i) {
      SendResult sendResult = producer.send(message);
      std::cout << sendResult.getMessageQueue().simpleName() << ": " << sendResult.getMsgId() << std::endl;
      count++;
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  } catch (...) {
    std::cerr << "Ah...No!!!" << std::endl;
  }

  stopped.store(true, std::memory_order_relaxed);
  if (stats_thread.joinable()) {
    stats_thread.join();
  }

  producer.shutdown();
  return EXIT_SUCCESS;
}