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

#include "StatsServerManager.h"
#include "RocketMQClient.h"
#include "StatsServer.h"
#include "string"

namespace rocketmq {
StatsServerManager::StatsServerManager() {
  serverName = "default";
}

StatsServerManager::~StatsServerManager() {
  m_consumeStatusServers.clear();
}
std::shared_ptr<StatsServer> StatsServerManager::getConsumeStatServer() {
  return getConsumeStatServer(serverName);
}
std::shared_ptr<StatsServer> StatsServerManager::getConsumeStatServer(std::string serverName) {
  std::map<std::string, std::shared_ptr<StatsServer>>::iterator it = m_consumeStatusServers.find(serverName);
  if (it != m_consumeStatusServers.end()) {
    return it->second;
  } else {
    std::shared_ptr<StatsServer> server = std::make_shared<StatsServer>();
    m_consumeStatusServers[serverName] = server;
    return server;
  }
}
void StatsServerManager::removeConsumeStatServer(std::string serverName) {
  std::map<std::string, std::shared_ptr<StatsServer>>::iterator it = m_consumeStatusServers.find(serverName);
  if (it != m_consumeStatusServers.end()) {
    m_consumeStatusServers.erase(it);
  }
}

StatsServerManager* StatsServerManager::getInstance() {
  static StatsServerManager instance;
  return &instance;
}
}  // namespace rocketmq
