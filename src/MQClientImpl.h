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
#ifndef __MQ_CLIENT_IMPL_H__
#define __MQ_CLIENT_IMPL_H__

#include "MQAdmin.h"
#include "MQClientConfig.h"
#include "MQClientInstance.h"
#include "ServiceState.h"

namespace rocketmq {

class MQClientImpl : public MQAdmin {
 public:
  MQClientImpl(MQClientConfigPtr config, RPCHookPtr rpcHook)
      : m_clientConfig(config), m_rpcHook(rpcHook), m_serviceState(CREATE_JUST), m_clientInstance(nullptr) {}

 public:  // MQAdmin
  void createTopic(const std::string& key, const std::string& newTopic, int queueNum) override;
  int64_t searchOffset(const MQMessageQueue& mq, uint64_t timestamp) override;
  int64_t maxOffset(const MQMessageQueue& mq) override;
  int64_t minOffset(const MQMessageQueue& mq) override;
  int64_t earliestMsgStoreTime(const MQMessageQueue& mq) override;
  MQMessageExtPtr viewMessage(const std::string& offsetMsgId) override;
  QueryResult queryMessage(const std::string& topic,
                           const std::string& key,
                           int maxNum,
                           int64_t begin,
                           int64_t end) override;

 public:
  virtual void start();
  virtual void shutdown();

  MQClientInstancePtr getFactory() const;
  virtual bool isServiceStateOk();

  RPCHookPtr getRPCHook() { return m_rpcHook; }
  void setRPCHook(RPCHookPtr rpcHook) { m_rpcHook = rpcHook; }

 protected:
  MQClientConfigPtr m_clientConfig;
  RPCHookPtr m_rpcHook;
  ServiceState m_serviceState;
  MQClientInstancePtr m_clientInstance;
};

}  // namespace rocketmq

#endif  // __MQ_CLIENT_IMPL_H__
