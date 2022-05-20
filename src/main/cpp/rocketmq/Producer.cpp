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
#include "rocketmq/Producer.h"

#include <chrono>
#include <memory>
#include <system_error>
#include <utility>

#include "DynamicNameServerResolver.h"
#include "LoggerImpl.h"
#include "MixAll.h"
#include "ProducerImpl.h"
#include "StaticNameServerResolver.h"
#include "absl/strings/str_split.h"
#include "rocketmq/ErrorCode.h"
#include "rocketmq/SendReceipt.h"
#include "rocketmq/Transaction.h"

ROCKETMQ_NAMESPACE_BEGIN

void Producer::start() {
  impl_->start();
}

SendReceipt Producer::send(MessageConstPtr message, std::error_code& ec) noexcept {
  if (!message) {
    ec = ErrorCode::BadRequest;
    return {};
  }

  return impl_->send(std::move(message), ec);
}

void Producer::send(MessageConstPtr message, const SendCallback& callback) noexcept {
  if (!message) {
    std::error_code ec = ErrorCode::BadRequest;
    SendReceipt send_receipt = {};
    callback(ec, send_receipt);
    return;
  }

  if (message->group().has_value()) {
    SendReceipt     empty;
    std::error_code ec = ErrorCode::BadRequestAsyncPubFifoMessage;
    callback(ec, empty);
    return;
  }

  impl_->send(std::move(message), callback);
}

ProducerBuilder Producer::newBuilder() {
  return {};
}

ProducerBuilder::ProducerBuilder() : impl_(std::make_shared<ProducerImpl>()){};

ProducerBuilder& ProducerBuilder::withConfiguration(Configuration configuration) {
  auto name_server_resolver = std::make_shared<StaticNameServerResolver>(configuration.endpoints());
  impl_->withNameServerResolver(std::move(name_server_resolver));
  impl_->withCredentialsProvider(configuration.credentialsProvider());
  impl_->withRequestTimeout(configuration.requestTimeout());
  return *this;
}

ProducerBuilder& ProducerBuilder::withTopics(const std::vector<std::string>& topics) {
  impl_->topicsOfInterest(topics);
  return *this;
}

ProducerBuilder& ProducerBuilder::withTransactionChecker(const TransactionChecker& checker) {
  return *this;
}

Producer ProducerBuilder::build() {
  Producer producer(impl_);
  producer.start();
  return producer;
}

ROCKETMQ_NAMESPACE_END