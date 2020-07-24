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
#include "DefaultMQProducer.h"

using namespace rocketmq;

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
  producer->setTcpTransportTryLockTimeout(1000);
  producer->setTcpTransportConnectTimeout(400);
  producer->start();

  MQMessage msg(info.topic,  // topic
                "*",         // tag
                info.body);  // body

  // messageDelayLevel=1s 5s 10s 30s 1m 2m 3m 4m 5m 6m 7m 8m 9m 10m 20m 30m 1h 2h
  msg.setDelayTimeLevel(5);  // 1m
  try {
    SendResult sendResult = producer->send(msg);
  } catch (const MQException& e) {
    std::cout << "send failed: " << std::endl;
  }

  producer->shutdown();

  delete producer;

  return 0;
}
