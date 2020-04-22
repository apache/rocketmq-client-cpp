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

#include "StatsServer.h"
#include "RocketMQClient.h"
#include "string"

namespace rocketmq {
const std::string StatsServer::TOPIC_AND_GROUP_CONSUME_OK_TPS = "CONSUME_OK_TPS";
const std::string StatsServer::TOPIC_AND_GROUP_CONSUME_FAILED_TPS = "CONSUME_FAILED_TPS";
const std::string StatsServer::TOPIC_AND_GROUP_CONSUME_RT = "CONSUME_RT";
const std::string StatsServer::TOPIC_AND_GROUP_PULL_TPS = "PULL_TPS";
const std::string StatsServer::TOPIC_AND_GROUP_PULL_RT = "PULL_RT";
StatsServer::StatsServer() {
  m_status = CREATE_JUST;
  serverName = "default";
}
StatsServer::StatsServer(std::string sName) {
  m_status = CREATE_JUST;
  serverName = sName;
}
StatsServer::~StatsServer() {}
void StatsServer::start() {
  switch (m_status) {
    case CREATE_JUST:
      m_status = RUNNING;
      break;
    case RUNNING:
    case SHUTDOWN_ALREADY:
    case START_FAILED:
      break;
    default:
      break;
  }
}
void StatsServer::shutdown() {
  switch (m_status) {
    case CREATE_JUST:
    case RUNNING:
      m_status = SHUTDOWN_ALREADY;
      break;
    case SHUTDOWN_ALREADY:
    case START_FAILED:
      break;
    default:
      break;
  }
}
ConsumeStats StatsServer::getConsumeStats(std::string topic, std::string groupName) {
  ConsumeStats consumeStats;
  consumeStats.consumeRT = 10;
  consumeStats.consumeOKTPS = 100;
  consumeStats.consumeFailedTPS = 20;
  consumeStats.consumeFailedMsgs = 1000;
  if (m_status == RUNNING) {
    std::string key = topic + "@" + groupName;
    std::lock_guard<std::mutex> lock(m_consumeStatusMutex);
    if (m_consumeStatus.find(key) != m_consumeStatus.end()) {
      return m_consumeStatus[key];
    }
    return consumeStats;
  }
  return consumeStats;
}
void StatsServer::incPullRT(std::string topic, std::string groupName, uint64 rt) {}
void StatsServer::incPullTPS(std::string topic, std::string groupName, uint64 msgCount) {}
void StatsServer::incConsumeRT(std::string topic, std::string groupName, uint64 rt) {}
void StatsServer::incConsumeOKTPS(std::string topic, std::string groupName, uint64 msgCount) {}
void StatsServer::incConsumeFailedTPS(std::string topic, std::string groupName, uint64 msgCount) {}

}  // namespace rocketmq
