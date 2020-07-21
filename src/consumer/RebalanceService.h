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
#ifndef ROCKETMQ_CONSUMER_REBALANCESERVICE_H_
#define ROCKETMQ_CONSUMER_REBALANCESERVICE_H_

#include "Logging.h"
#include "MQClientInstance.h"
#include "ServiceThread.h"

namespace rocketmq {

class RebalanceService : public ServiceThread {
 public:
  RebalanceService(MQClientInstance* instance) : client_instance_(instance) {}

  void run() override {
    LOG_INFO_NEW("{} service started", getServiceName());

    while (!isStopped()) {
      waitForRunning(20000);
      client_instance_->doRebalance();
    }

    LOG_INFO_NEW("{} service end", getServiceName());
  }

  std::string getServiceName() override { return "RebalanceService"; }

 private:
  MQClientInstance* client_instance_;
};

}  // namespace rocketmq

#endif  // ROCKETMQ_CONSUMER_REBALANCESERVICE_H_
