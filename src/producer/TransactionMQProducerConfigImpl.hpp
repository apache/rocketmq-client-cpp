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
#ifndef ROCKETMQ_PRODUCER_TRANSACTIONMQPRODUCERCONFIGIMPL_HPP_
#define ROCKETMQ_PRODUCER_TRANSACTIONMQPRODUCERCONFIGIMPL_HPP_

#include "DefaultMQProducerConfigImpl.hpp"
#include "TransactionMQProducerConfig.h"

namespace rocketmq {

class TransactionMQProducerConfigImpl : virtual public TransactionMQProducerConfig, public DefaultMQProducerConfigImpl {
 public:
  TransactionMQProducerConfigImpl() : transaction_listener_(nullptr) {}
  virtual ~TransactionMQProducerConfigImpl() = default;

 public:  // TransactionMQProducerConfig
  TransactionListener* getTransactionListener() const override { return transaction_listener_; }
  void setTransactionListener(TransactionListener* transactionListener) override {
    transaction_listener_ = transactionListener;
  }

 protected:
  TransactionListener* transaction_listener_;
};

}  // namespace rocketmq

#endif  // ROCKETMQ_PRODUCER_TRANSACTIONMQPRODUCERCONFIGIMPL_HPP_
