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
#include "MQClientManager.h"

#include "Logging.h"

namespace rocketmq {

MQClientManager* MQClientManager::getInstance() {
  static MQClientManager singleton_;
  return &singleton_;
}

MQClientManager::MQClientManager() = default;
MQClientManager::~MQClientManager() = default;

MQClientInstancePtr MQClientManager::getOrCreateMQClientInstance(const MQClientConfig& clientConfig) {
  return getOrCreateMQClientInstance(clientConfig, nullptr);
}

MQClientInstancePtr MQClientManager::getOrCreateMQClientInstance(const MQClientConfig& clientConfig,
                                                                 RPCHookPtr rpcHook) {
  std::string clientId = clientConfig.buildMQClientId();
  std::lock_guard<std::mutex> lock(mutex_);
  const auto& it = instance_table_.find(clientId);
  if (it != instance_table_.end()) {
    return it->second;
  } else {
    // Clone clientConfig in Java, but we don't now.
    auto instance = std::make_shared<MQClientInstance>(clientConfig, clientId, rpcHook);
    instance_table_[clientId] = instance;
    LOG_INFO_NEW("Created new MQClientInstance for clientId:[{}]", clientId);
    return instance;
  }
}

void MQClientManager::removeMQClientInstance(const std::string& clientId) {
  std::lock_guard<std::mutex> lock(mutex_);
  const auto& it = instance_table_.find(clientId);
  if (it != instance_table_.end()) {
    instance_table_.erase(it);
  }
}

}  // namespace rocketmq
