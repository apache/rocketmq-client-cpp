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

#ifndef __CONSUMER_STATUS_SERVICE_H__
#define __CONSUMER_STATUS_SERVICE_H__

#include <boost/asio.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/system/error_code.hpp>
#include <boost/thread/thread.hpp>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include "ConsumeStats.h"
#include "RocketMQClient.h"
#include "ServiceState.h"
#include "StatsItem.h"

namespace rocketmq {
class StatsServer {
 public:
  static const std::string TOPIC_AND_GROUP_CONSUME_OK_TPS;
  static const std::string TOPIC_AND_GROUP_CONSUME_FAILED_TPS;
  static const std::string TOPIC_AND_GROUP_CONSUME_RT;
  static const std::string TOPIC_AND_GROUP_PULL_TPS;
  static const std::string TOPIC_AND_GROUP_PULL_RT;
  static const int SAMPLING_PERIOD;
  StatsServer();
  StatsServer(std::string serverName);
  virtual ~StatsServer();
  void start();
  void shutdown();
  ConsumeStats getConsumeStats(std::string topic, std::string groupName);
  void incPullRT(std::string topic, std::string groupName, uint64 rt);
  void incPullTPS(std::string topic, std::string groupName, uint64 msgCount);
  void incConsumeRT(std::string topic, std::string groupName, uint64 rt, uint64 msgCount = 1);
  void incConsumeOKTPS(std::string topic, std::string groupName, uint64 msgCount);
  void incConsumeFailedTPS(std::string topic, std::string groupName, uint64 msgCount);
  void incConsumeFailedMsgs(std::string topic, std::string groupName, uint64 msgCount);

 private:
  void startScheduledTask();
  void stopScheduledTask();
  void doStartScheduledTask();
  void scheduledTaskInSeconds(boost::system::error_code& ec, std::shared_ptr<boost::asio::deadline_timer> t);
  void samplingInSeconds();
  void updateConsumeStats(std::string topic, std::string groupName, ConsumeStats consumeStats);
  void updateConsumeStats(std::string key, ConsumeStats consumeStats);
  ConsumeStats getConsumeStats(std::string key);

 public:
  std::string serverName;

 private:
  ServiceState m_status;
  std::mutex m_consumeStatusMutex;
  std::map<std::string, ConsumeStats> m_consumeStatus;
  boost::asio::io_service m_consumerStatus_ioService;
  // boost::asio::io_service::work m_consumerStatus_work;
  std::unique_ptr<boost::thread> m_consumer_status_service_thread;
  std::mutex m_consumeStatsItemMutex;
  std::map<std::string, StatsItem> m_consumeStatsItems;
};

}  // namespace rocketmq

#endif
