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

#ifndef __CONSUMER_STATUS_SERVICE_MANAGER_H__
#define __CONSUMER_STATUS_SERVICE_MANAGER_H__

#include <map>
#include <memory>
#include <string>
#include "ConsumeStats.h"
#include "RocketMQClient.h"
#include "ServiceState.h"
#include "StatsServer.h"

namespace rocketmq {
class StatsServerManager {
 public:
  virtual ~StatsServerManager();
  // void start();
  // void shutdown();
  std::shared_ptr<StatsServer> getConsumeStatServer();
  std::shared_ptr<StatsServer> getConsumeStatServer(std::string serverName);
  void removeConsumeStatServer(std::string serverName);

  static StatsServerManager* getInstance();

 private:
  StatsServerManager();

 public:
  std::string serverName;

 private:
  std::map<std::string, std::shared_ptr<StatsServer>> m_consumeStatusServers;
};

}  // namespace rocketmq

#endif
