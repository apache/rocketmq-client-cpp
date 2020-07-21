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
#ifndef ROCKETMQ_TRANSACTIONMQPRODUCER_H_
#define ROCKETMQ_TRANSACTIONMQPRODUCER_H_

#include "DefaultMQProducer.h"
#include "TransactionMQProducerConfig.h"

namespace rocketmq {

class ROCKETMQCLIENT_API TransactionMQProducer : public DefaultMQProducer,                   // base
                                                 virtual public TransactionMQProducerConfig  // interface
{
 public:
  TransactionMQProducer(const std::string& groupname);
  TransactionMQProducer(const std::string& groupname, RPCHookPtr rpcHook);
  virtual ~TransactionMQProducer();

 public:  // MQProducer
  void start() override;
  void shutdown() override;

  // Transaction
  TransactionSendResult sendMessageInTransaction(MQMessage& msg, void* arg) override;

 public:  // TransactionMQProducerConfig
  TransactionListener* getTransactionListener() const override;
  void setTransactionListener(TransactionListener* transactionListener) override;

 public:  // DefaultMQProducerConfigProxy
  inline TransactionMQProducerConfigPtr real_config() const {
    return std::dynamic_pointer_cast<TransactionMQProducer>(client_config_);
  }
};

}  // namespace rocketmq

#endif  // ROCKETMQ_TRANSACTIONMQPRODUCER_H_
