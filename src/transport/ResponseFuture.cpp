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
#include "ResponseFuture.h"

#include "UtilAll.h"

namespace rocketmq {

ResponseFuture::ResponseFuture(int requestCode,
                               int opaque,
                               int64_t timeoutMillis,
                               std::unique_ptr<InvokeCallback> invokeCallback)
    : request_code_(requestCode),
      opaque_(opaque),
      timeout_millis_(timeoutMillis),
      invoke_callback_(std::move(invokeCallback)),
      begin_timestamp_(UtilAll::currentTimeMillis()),
      send_request_ok_(false),
      response_command_(nullptr),
      count_down_latch_(nullptr) {
  if (nullptr == invokeCallback) {
    count_down_latch_.reset(new latch(1));
  }
}

ResponseFuture::~ResponseFuture() = default;

void ResponseFuture::releaseThreadCondition() {
  if (count_down_latch_ != nullptr) {
    count_down_latch_->count_down();
  }
}

bool ResponseFuture::hasInvokeCallback() {
  // if invoke_callback_ is set, this is an async future.
  return invoke_callback_ != nullptr;
}

void ResponseFuture::executeInvokeCallback() noexcept {
  if (invoke_callback_ != nullptr) {
    invoke_callback_->operationComplete(this);
  }
}

std::unique_ptr<RemotingCommand> ResponseFuture::waitResponse(int timeoutMillis) {
  if (count_down_latch_ != nullptr) {
    if (timeoutMillis < 0) {
      timeoutMillis = 0;
    }
    count_down_latch_->wait(timeoutMillis, time_unit::milliseconds);
  }
  return std::move(response_command_);
}

void ResponseFuture::putResponse(std::unique_ptr<RemotingCommand> responseCommand) {
  response_command_ = std::move(responseCommand);
  if (count_down_latch_ != nullptr) {
    count_down_latch_->count_down();
  }
}

std::unique_ptr<RemotingCommand> ResponseFuture::getResponseCommand() {
  return std::move(response_command_);
}

void ResponseFuture::setResponseCommand(std::unique_ptr<RemotingCommand> responseCommand) {
  response_command_ = std::move(responseCommand);
}

bool ResponseFuture::isTimeout() const {
  auto diff = UtilAll::currentTimeMillis() - begin_timestamp_;
  return diff > timeout_millis_;
}

int64_t ResponseFuture::leftTime() const {
  auto diff = UtilAll::currentTimeMillis() - begin_timestamp_;
  auto left = timeout_millis_ - diff;
  return left < 0 ? 0 : left;
}

}  // namespace rocketmq
