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
#ifndef __REBALANCE_SERVICE_H__
#define __REBALANCE_SERVICE_H__

#include "Logging.h"
#include "MQClientInstance.h"
#include "ServiceThread.h"

namespace rocketmq {

class RebalanceService : public ServiceThread {
 public:
  RebalanceService(MQClientInstance* instance) : m_clientInstance(instance) {}

  void run() override {
    LOG_INFO_NEW("{} service started", getServiceName());

    while (!isStopped()) {
      waitForRunning(20000);
      m_clientInstance->doRebalance();
    }

    LOG_INFO_NEW("{} service end", getServiceName());
  }

  std::string getServiceName() override { return "RebalanceService"; }

 private:
  MQClientInstance* m_clientInstance;
};

}  // namespace rocketmq

#endif  // __REBALANCE_SERVICE_H__
