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

MQClientInstance* MQClientManager::getAndCreateMQClientInstance(MQClient* clientConfig) {
  return getAndCreateMQClientInstance(clientConfig, nullptr);
}

MQClientInstance* MQClientManager::getAndCreateMQClientInstance(MQClient* clientConfig,
                                                                std::shared_ptr<RPCHook> rpcHook) {
  std::string clientId = clientConfig->buildMQClientId();
  std::lock_guard<std::mutex> lock(m_mutex);
  auto it = m_factoryTable.find(clientId);
  if (it != m_factoryTable.end()) {
    return it->second;
  } else {
    auto* factory = new MQClientInstance(clientConfig, clientId, rpcHook);
    m_factoryTable[clientId] = factory;
    LOG_INFO_NEW("Created new MQClientInstance for clientId:[{}]", clientId);
    return factory;
  }
}

void MQClientManager::removeMQClientInstance(const std::string& clientId) {
  MQClientInstance* instance = nullptr;
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    FTMAP::iterator it = m_factoryTable.find(clientId);
    if (it != m_factoryTable.end()) {
      instance = it->second;
      m_factoryTable.erase(it);
    }
  }
  delete instance;
}

}  // namespace rocketmq
