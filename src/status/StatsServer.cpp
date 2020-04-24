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
#include <memory>
#include <string>
#include "Logging.h"
#include "RocketMQClient.h"
#include "StatsItem.h"

namespace rocketmq {
const std::string StatsServer::TOPIC_AND_GROUP_CONSUME_OK_TPS = "CONSUME_OK_TPS";
const std::string StatsServer::TOPIC_AND_GROUP_CONSUME_FAILED_TPS = "CONSUME_FAILED_TPS";
const std::string StatsServer::TOPIC_AND_GROUP_CONSUME_RT = "CONSUME_RT";
const std::string StatsServer::TOPIC_AND_GROUP_PULL_TPS = "PULL_TPS";
const std::string StatsServer::TOPIC_AND_GROUP_PULL_RT = "PULL_RT";
const int StatsServer::SAMPLING_PERIOD = 10;
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
      m_status = START_FAILED;
      startScheduledTask();
      LOG_INFO("Default Status Service Start.");
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
      LOG_INFO("Default Status Service ShutDown.");
      stopScheduledTask();
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
  LOG_DEBUG("getConsumeStats Topic:%s, Group:%s", topic.c_str(), groupName.c_str());
  if (m_status == RUNNING) {
    std::string key = topic + "@" + groupName;
    LOG_DEBUG("getConsumeStats Key:%s", key.c_str());
    return getConsumeStats(key);
  }
  return consumeStats;
}
void StatsServer::incPullRT(std::string topic, std::string groupName, uint64 rt) {
  std::string key = topic + "@" + groupName;
  std::lock_guard<std::mutex> lock(m_consumeStatsItemMutex);
  if (m_consumeStatsItems.find(key) == m_consumeStatsItems.end()) {
    StatsItem item;
    m_consumeStatsItems[key] = item;
  }
  m_consumeStatsItems[key].pullRT += rt;
  m_consumeStatsItems[key].pullRTCount += 1;
}
void StatsServer::incPullTPS(std::string topic, std::string groupName, uint64 msgCount) {
  std::string key = topic + "@" + groupName;
  std::lock_guard<std::mutex> lock(m_consumeStatsItemMutex);
  if (m_consumeStatsItems.find(key) == m_consumeStatsItems.end()) {
    StatsItem item;
    m_consumeStatsItems[key] = item;
  }
  m_consumeStatsItems[key].pullCount += msgCount;
}
void StatsServer::incConsumeRT(std::string topic, std::string groupName, uint64 rt, uint64 msgCount) {
  std::string key = topic + "@" + groupName;
  LOG_DEBUG("incConsumeRT before Key:%s, RT:%lld, Count: %lld", key.c_str(), rt, msgCount);

  std::lock_guard<std::mutex> lock(m_consumeStatsItemMutex);
  if (m_consumeStatsItems.find(key) == m_consumeStatsItems.end()) {
    StatsItem item;
    m_consumeStatsItems[key] = item;
  }
  m_consumeStatsItems[key].consumeRT += rt;
  m_consumeStatsItems[key].consumeRTCount += msgCount;
  LOG_DEBUG("incConsumeRT After Key:%s, RT:%lld, Count: %lld", key.c_str(), m_consumeStatsItems[key].consumeRT,
            m_consumeStatsItems[key].consumeRTCount);
}
void StatsServer::incConsumeOKTPS(std::string topic, std::string groupName, uint64 msgCount) {
  std::string key = topic + "@" + groupName;
  LOG_DEBUG("incConsumeOKTPS Before Key:%s, Count: %lld", key.c_str(), msgCount);
  std::lock_guard<std::mutex> lock(m_consumeStatsItemMutex);
  if (m_consumeStatsItems.find(key) == m_consumeStatsItems.end()) {
    StatsItem item;
    m_consumeStatsItems[key] = item;
  }
  m_consumeStatsItems[key].consumeOKCount += msgCount;
  LOG_DEBUG("incConsumeOKTPS After Key:%s, Count: %lld", key.c_str(), m_consumeStatsItems[key].consumeOKCount);
}
void StatsServer::incConsumeFailedTPS(std::string topic, std::string groupName, uint64 msgCount) {
  std::string key = topic + "@" + groupName;
  LOG_DEBUG("incConsumeFailedTPS Key:%s, Count: %lld", key.c_str(), msgCount);
  std::lock_guard<std::mutex> lock(m_consumeStatsItemMutex);
  if (m_consumeStatsItems.find(key) == m_consumeStatsItems.end()) {
    StatsItem item;
    m_consumeStatsItems[key] = item;
  }
  m_consumeStatsItems[key].consumeFailedCount += msgCount;
  m_consumeStatsItems[key].consumeFailedMsgs += msgCount;
}
void StatsServer::incConsumeFailedMsgs(std::string topic, std::string groupName, uint64 msgCount) {
  std::string key = topic + "@" + groupName;
  LOG_DEBUG("incConsumeFailedTPS Key:%s, Count: %lld", key.c_str(), msgCount);
  std::lock_guard<std::mutex> lock(m_consumeStatsItemMutex);
  if (m_consumeStatsItems.find(key) == m_consumeStatsItems.end()) {
    StatsItem item;
    m_consumeStatsItems[key] = item;
  }
  m_consumeStatsItems[key].consumeFailedMsgs += msgCount;
}
void StatsServer::startScheduledTask() {
  m_consumer_status_service_thread.reset(new boost::thread(boost::bind(&StatsServer::doStartScheduledTask, this)));
}
void StatsServer::stopScheduledTask() {
  if (m_consumer_status_service_thread) {
    m_consumerStatus_ioService.stop();
    m_consumer_status_service_thread->interrupt();
    m_consumer_status_service_thread->join();
    m_consumer_status_service_thread.reset();
  }
}
void StatsServer::doStartScheduledTask() {
  boost::asio::io_service::work work(m_consumerStatus_ioService);
  boost::system::error_code ec1;
  std::shared_ptr<boost::asio::deadline_timer> t1 = std::make_shared<boost::asio::deadline_timer>(
      m_consumerStatus_ioService, boost::posix_time::seconds(SAMPLING_PERIOD));
  t1->async_wait(boost::bind(&StatsServer::scheduledTaskInSeconds, this, ec1, t1));

  boost::system::error_code errorCode;
  m_consumerStatus_ioService.run(errorCode);
}
void StatsServer::scheduledTaskInSeconds(boost::system::error_code& ec,
                                         std::shared_ptr<boost::asio::deadline_timer> t) {
  samplingInSeconds();

  boost::system::error_code e;
  t->expires_from_now(t->expires_from_now() + boost::posix_time::seconds(SAMPLING_PERIOD), e);
  t->async_wait(boost::bind(&StatsServer::scheduledTaskInSeconds, this, ec, t));
}
void StatsServer::samplingInSeconds() {
  LOG_DEBUG("samplingInSeconds==");

  // do samplings
  std::lock_guard<std::mutex> lock(m_consumeStatsItemMutex);
  for (std::map<std::string, StatsItem>::iterator it = m_consumeStatsItems.begin(); it != m_consumeStatsItems.end();
       ++it) {
    ConsumeStats consumeStats;
    if (it->second.pullRTCount != 0) {
      consumeStats.pullRT = (1.0 * (it->second.pullRT)) / (it->second.pullRTCount);
    }
    it->second.pullRT = 0;
    it->second.pullRTCount = 0;
    consumeStats.pullTPS = (1.0 * (it->second.pullCount)) / SAMPLING_PERIOD;
    it->second.pullCount = 0;
    if (it->second.consumeRTCount != 0) {
      consumeStats.consumeRT = (1.0 * (it->second.consumeRT)) / (it->second.consumeRTCount);
      LOG_DEBUG("samplingInSeconds Key[%s], consumeRT:%.2f,Total RT:%lld, Count: %lld", it->first.c_str(),
                consumeStats.consumeRT, it->second.consumeRT, it->second.consumeRTCount);
    }
    it->second.consumeRT = 0;
    it->second.consumeRTCount = 0;
    consumeStats.consumeOKTPS = (1.0 * (it->second.consumeOKCount)) / SAMPLING_PERIOD;
    LOG_DEBUG("samplingInSeconds Key[%s], consumeOKTPS:%.2f, Count: %lld", it->first.c_str(), consumeStats.consumeOKTPS,
              it->second.consumeOKCount);
    it->second.consumeOKCount = 0;
    consumeStats.consumeFailedTPS = (1.0 * (it->second.consumeFailedCount)) / SAMPLING_PERIOD;
    it->second.consumeFailedCount = 0;
    LOG_DEBUG("samplingInSeconds Key[%s], consumeFailedTPS:%.2f, Count: %lld", it->first.c_str(),
              consumeStats.consumeFailedTPS, it->second.consumeFailedCount);
    consumeStats.consumeFailedMsgs = it->second.consumeFailedMsgs;
    // it->second.consumeFailedMsgs = 0;
    updateConsumeStats(it->first, consumeStats);
  }
}

void StatsServer::updateConsumeStats(std::string topic, std::string groupName, ConsumeStats consumeStats) {
  if (m_status == RUNNING) {
    std::string key = topic + "@" + groupName;
    updateConsumeStats(key, consumeStats);
  }
}
void StatsServer::updateConsumeStats(std::string key, ConsumeStats consumeStats) {
  LOG_DEBUG("updateConsumeStats Key:%s, Count: %lld", key.c_str(), consumeStats.consumeOKTPS);

  std::lock_guard<std::mutex> lock(m_consumeStatusMutex);
  m_consumeStatus[key] = consumeStats;
}
ConsumeStats StatsServer::getConsumeStats(std::string key) {
  ConsumeStats consumeStats;
  std::lock_guard<std::mutex> lock(m_consumeStatusMutex);
  if (m_consumeStatus.find(key) != m_consumeStatus.end()) {
    return m_consumeStatus[key];
  }
  return consumeStats;
}
}  // namespace rocketmq
