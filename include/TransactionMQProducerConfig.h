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
#ifndef ROCKETMQ_TRANSACTIONMQPRODUCERCONFIG_H_
#define ROCKETMQ_TRANSACTIONMQPRODUCERCONFIG_H_

#include "DefaultMQProducerConfig.h"
#include "TransactionListener.h"

namespace rocketmq {

class TransactionMQProducerConfig;
typedef std::shared_ptr<TransactionMQProducerConfig> TransactionMQProducerConfigPtr;

/**
 * TransactionMQProducerConfig - config interface for TransactionMQProducer
 */
class ROCKETMQCLIENT_API TransactionMQProducerConfig : virtual public DefaultMQProducerConfig {
 public:
  virtual ~TransactionMQProducerConfig() = default;

 public:  // TransactionMQProducerConfig
  virtual TransactionListener* getTransactionListener() const = 0;
  virtual void setTransactionListener(TransactionListener* transactionListener) = 0;
};

}  // namespace rocketmq

#endif  // ROCKETMQ_TRANSACTIONMQPRODUCERCONFIG_H_
