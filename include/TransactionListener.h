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
#ifndef ROCKETMQ_TRANSACTIONLISTENER_H_
#define ROCKETMQ_TRANSACTIONLISTENER_H_

#include "MQMessageExt.h"
#include "TransactionSendResult.h"

namespace rocketmq {

/**
 * TransactionListener - listener interface for TransactionMQProducer
 */
class ROCKETMQCLIENT_API TransactionListener {
 public:
  virtual ~TransactionListener() = default;

  /**
   * When send transactional prepare(half) message succeed, this method will be invoked to execute local transaction.
   *
   * @param msg Half(prepare) message
   * @param arg Custom business parameter
   * @return Transaction state
   */
  virtual LocalTransactionState executeLocalTransaction(const MQMessage& msg, void* arg) = 0;

  /**
   * When no response to prepare(half) message. broker will send check message to check the transaction status, and this
   * method will be invoked to get local transaction status.
   *
   * @param msg Check message
   * @return Transaction state
   */
  virtual LocalTransactionState checkLocalTransaction(const MQMessageExt& msg) = 0;
};

}  // namespace rocketmq

#endif  // ROCKETMQ_TRANSACTIONLISTENER_H_
