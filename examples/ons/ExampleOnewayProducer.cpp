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
#include "ons/ONSFactory.h"
#include "rocketmq/Logger.h"
#include <chrono>
#include <iostream>
#include <thread>

using namespace std;
using namespace ons;

int main(int argc, char* argv[]) {
  rocketmq::Logger& logger = rocketmq::getLogger();
  logger.setLevel(rocketmq::Level::Debug);
  logger.init();

  std::cout << "=======Before sending messages=======" << std::endl;
  ONSFactoryProperty factoryInfo;

  /*
    factoryInfo.setFactoryProperty(ONSFactoryProperty::GroupId, "Your-GroupId");
    factoryInfo.setFactoryProperty(ONSFactoryProperty::AccessKey, "Your-Access-Key");
    factoryInfo.setFactoryProperty(ONSFactoryProperty::SecretKey, "Your-Secret-Key");
    factoryInfo.setFactoryProperty(ONSFactoryProperty::NAMESRV_ADDR, "Your-Access-Point-URL");
  */

  Producer* producer = nullptr;
  producer = ONSFactory::getInstance()->createProducer(factoryInfo);
  producer->start();
  Message msg("cpp_sdk_standard", "tagA", "ORDERID_100", "hello MQ_lingchu");

  auto start = std::chrono::system_clock::now();
  int count = 32;
  for (int i = 0; i < count; ++i) {
    producer->sendOneway(msg);
  }
  auto interval = std::chrono::system_clock::now() - start;
  std::cout << "Send " << count << " messages OK, costs "
            << std::chrono::duration_cast<std::chrono::milliseconds>(interval).count() << "ms" << std::endl;

  std::this_thread::sleep_for(std::chrono::seconds(1));
  producer->shutdown();
  std::cout << "=======After sending messages=======" << std::endl;
  return 0;
}