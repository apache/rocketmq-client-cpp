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
#ifndef ROCKETMQ_MQCLIENTIMPL_H_
#define ROCKETMQ_MQCLIENTIMPL_H_

#include "MQAdmin.h"
#include "MQClientConfig.h"
#include "MQClientInstance.h"
#include "ServiceState.h"

namespace rocketmq {

class MQClientImpl : public MQAdmin {
 public:
  MQClientImpl(MQClientConfigPtr config, RPCHookPtr rpcHook)
      : client_config_(config), rpc_hook_(rpcHook), service_state_(CREATE_JUST), client_instance_(nullptr) {}

 public:  // MQAdmin
  void createTopic(const std::string& key, const std::string& newTopic, int queueNum) override;
  int64_t searchOffset(const MQMessageQueue& mq, int64_t timestamp) override;
  int64_t maxOffset(const MQMessageQueue& mq) override;
  int64_t minOffset(const MQMessageQueue& mq) override;
  int64_t earliestMsgStoreTime(const MQMessageQueue& mq) override;
  MQMessageExt viewMessage(const std::string& offsetMsgId) override;
  QueryResult queryMessage(const std::string& topic,
                           const std::string& key,
                           int maxNum,
                           int64_t begin,
                           int64_t end) override;

 public:
  virtual void start();
  virtual void shutdown();

  virtual bool isServiceStateOk();

  MQClientInstancePtr getClientInstance() const;
  void setClientInstance(MQClientInstancePtr clientInstance);

  RPCHookPtr getRPCHook() { return rpc_hook_; }
  void setRPCHook(RPCHookPtr rpcHook) { rpc_hook_ = rpcHook; }

 protected:
  MQClientConfigPtr client_config_;
  RPCHookPtr rpc_hook_;
  volatile ServiceState service_state_;
  MQClientInstancePtr client_instance_;
};

}  // namespace rocketmq

#endif  // ROCKETMQ_MQCLIENTIMPL_H_
