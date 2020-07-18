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
#ifndef __TRANSACTION_MQ_PRODUCER_H__
#define __TRANSACTION_MQ_PRODUCER_H__

#include "DefaultMQProducer.h"
#include "TransactionMQProducerConfig.h"

namespace rocketmq {

class ROCKETMQCLIENT_API TransactionMQProducer : public DefaultMQProducer, virtual public TransactionMQProducerConfig {
 public:
  TransactionMQProducer(const std::string& groupname);
  TransactionMQProducer(const std::string& groupname, RPCHookPtr rpcHook);
  virtual ~TransactionMQProducer();

 public:  // TransactionMQProducerConfig
  TransactionListener* getTransactionListener() const override;
  void setTransactionListener(TransactionListener* transactionListener) override;

 public:  // MQProducer
  void start() override;
  void shutdown() override;

  // Transaction: don't delete msg object, until callback occur.
  TransactionSendResult sendMessageInTransaction(MQMessage& msg, void* arg) override;
};

}  // namespace rocketmq

#endif  // __TRANSACTION_MQ_PRODUCER_H__
