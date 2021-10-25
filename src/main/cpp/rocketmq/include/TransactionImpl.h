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
#pragma once

#include <memory>
#include <string>

#include "rocketmq/MQMessage.h"
#include "rocketmq/Transaction.h"

ROCKETMQ_NAMESPACE_BEGIN

class ProducerImpl;

class TransactionImpl : public Transaction {
public:
  TransactionImpl(MQMessage message, std::string transaction_id, std::string endpoint, std::string trace_context,
                  const std::shared_ptr<ProducerImpl>& producer)
      : message_(std::move(message)), transaction_id_(std::move(transaction_id)), endpoint_(std::move(endpoint)),
        trace_context_(std::move(trace_context)), producer_(producer) {
  }

  ~TransactionImpl() override = default;

  bool commit() override;

  bool rollback() override;

  std::string messageId() const override;

  std::string transactionId() const override;

private:
  MQMessage message_;
  std::string transaction_id_;
  std::string endpoint_;
  std::string trace_context_;
  std::weak_ptr<ProducerImpl> producer_;
};

ROCKETMQ_NAMESPACE_END