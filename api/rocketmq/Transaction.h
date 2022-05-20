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

#include <cstdint>
#include <memory>
#include <string>

#include "RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

class Transaction {
public:
  Transaction() = default;

  virtual ~Transaction() = default;

  virtual bool commit() = 0;

  virtual bool rollback() = 0;

  virtual const std::string& topic() const = 0;

  virtual const std::string& messageId() const = 0;

  virtual const std::string& transactionId() const = 0;

  virtual const std::string& traceContext() const = 0;

  virtual const std::string& endpoint() const = 0;
};

using TransactionPtr = std::unique_ptr<Transaction>;
using TransactionConstPtr = std::unique_ptr<const Transaction>;

ROCKETMQ_NAMESPACE_END