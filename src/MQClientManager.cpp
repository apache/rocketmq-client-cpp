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

MQClientInstancePtr MQClientManager::getOrCreateMQClientInstance(ConstMQClientConfigPtr clientConfig) {
  return getOrCreateMQClientInstance(clientConfig, nullptr);
}

MQClientInstancePtr MQClientManager::getOrCreateMQClientInstance(ConstMQClientConfigPtr clientConfig,
                                                                 RPCHookPtr rpcHook) {
  std::string clientId = clientConfig->buildMQClientId();
  std::lock_guard<std::mutex> lock(m_mutex);
  const auto& it = m_instanceTable.find(clientId);
  if (it != m_instanceTable.end()) {
    return it->second;
  } else {
    // clone clientConfig
    auto instance = std::make_shared<MQClientInstance>(clientConfig, clientId, rpcHook);
    m_instanceTable[clientId] = instance;
    LOG_INFO_NEW("Created new MQClientInstance for clientId:[{}]", clientId);
    return instance;
  }
}

void MQClientManager::removeMQClientInstance(const std::string& clientId) {
  std::lock_guard<std::mutex> lock(m_mutex);
  const auto& it = m_instanceTable.find(clientId);
  if (it != m_instanceTable.end()) {
    m_instanceTable.erase(it);
  }
}

}  // namespace rocketmq
