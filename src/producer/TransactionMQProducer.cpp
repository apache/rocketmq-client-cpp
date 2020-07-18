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
#include "TransactionMQProducerConfigImpl.h"

namespace rocketmq {

TransactionMQProducer::TransactionMQProducer(const std::string& groupname)
    : TransactionMQProducer(groupname, nullptr) {}

TransactionMQProducer::TransactionMQProducer(const std::string& groupname, RPCHookPtr rpcHook)
    : DefaultMQProducer(groupname, rpcHook, std::make_shared<TransactionMQProducerConfigImpl>()) {}

TransactionMQProducer::~TransactionMQProducer() = default;

TransactionListener* TransactionMQProducer::getTransactionListener() const {
  auto transactionProducerConfig = std::dynamic_pointer_cast<TransactionMQProducerConfig>(getRealConfig());
  if (transactionProducerConfig != nullptr) {
    return transactionProducerConfig->getTransactionListener();
  }
  return nullptr;
}

void TransactionMQProducer::setTransactionListener(TransactionListener* transactionListener) {
  auto transactionProducerConfig = std::dynamic_pointer_cast<TransactionMQProducerConfig>(getRealConfig());
  if (transactionProducerConfig != nullptr) {
    transactionProducerConfig->setTransactionListener(transactionListener);
  }
}

void TransactionMQProducer::start() {
  std::dynamic_pointer_cast<DefaultMQProducerImpl>(m_producerDelegate)->initTransactionEnv();
  DefaultMQProducer::start();
}

void TransactionMQProducer::shutdown() {
  DefaultMQProducer::shutdown();
  std::dynamic_pointer_cast<DefaultMQProducerImpl>(m_producerDelegate)->destroyTransactionEnv();
}

TransactionSendResult TransactionMQProducer::sendMessageInTransaction(MQMessage& msg, void* arg) {
  return m_producerDelegate->sendMessageInTransaction(msg, arg);
}

}  // namespace rocketmq
