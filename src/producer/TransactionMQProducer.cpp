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
#include "TransactionMQProducer.h"

#include "DefaultMQProducerImpl.h"
#include "TransactionMQProducerConfigImpl.hpp"

namespace rocketmq {

TransactionMQProducer::TransactionMQProducer(const std::string& groupname)
    : TransactionMQProducer(groupname, nullptr) {}

TransactionMQProducer::TransactionMQProducer(const std::string& groupname, RPCHookPtr rpcHook)
    : DefaultMQProducer(groupname, rpcHook, std::make_shared<TransactionMQProducerConfigImpl>()) {}

TransactionMQProducer::~TransactionMQProducer() = default;

void TransactionMQProducer::start() {
  dynamic_cast<DefaultMQProducerImpl*>(producer_impl_.get())->initTransactionEnv();
  DefaultMQProducer::start();
}

void TransactionMQProducer::shutdown() {
  DefaultMQProducer::shutdown();
  dynamic_cast<DefaultMQProducerImpl*>(producer_impl_.get())->destroyTransactionEnv();
}

TransactionSendResult TransactionMQProducer::sendMessageInTransaction(MQMessage& msg, void* arg) {
  return producer_impl_->sendMessageInTransaction(msg, arg);
}

TransactionListener* TransactionMQProducer::getTransactionListener() const {
  auto transactionProducerConfig = dynamic_cast<TransactionMQProducerConfig*>(client_config_.get());
  if (transactionProducerConfig != nullptr) {
    return transactionProducerConfig->getTransactionListener();
  }
  return nullptr;
}

void TransactionMQProducer::setTransactionListener(TransactionListener* transactionListener) {
  auto transactionProducerConfig = dynamic_cast<TransactionMQProducerConfig*>(client_config_.get());
  if (transactionProducerConfig != nullptr) {
    transactionProducerConfig->setTransactionListener(transactionListener);
  }
}

}  // namespace rocketmq
