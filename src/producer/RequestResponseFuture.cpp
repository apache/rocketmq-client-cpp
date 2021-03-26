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
#include "RequestResponseFuture.h"

#include "Logging.h"
#include "UtilAll.h"

namespace rocketmq {

void RequestCallback::invokeOnSuccess(MQMessage message) noexcept {
  auto type = getRequestCallbackType();
  try {
    onSuccess(std::move(message));
  } catch (const std::exception& e) {
    LOG_WARN_NEW("encounter exception when invoke RequestCallback::onSuccess(), {}", e.what());
  }
  if (type == RequestCallbackType::kAutoDelete) {
    delete this;
  }
}

void RequestCallback::invokeOnException(MQException& exception) noexcept {
  auto type = getRequestCallbackType();
  try {
    onException(exception);
  } catch (const std::exception& e) {
    LOG_WARN_NEW("encounter exception when invoke RequestCallback::onException(), {}", e.what());
  }
  if (type == RequestCallbackType::kAutoDelete) {
    delete this;
  }
}

RequestResponseFuture::RequestResponseFuture(const std::string& correlationId,
                                             long timeoutMillis,
                                             RequestCallback* requestCallback)
    : correlation_id_(correlationId),
      request_callback_(requestCallback),
      begin_timestamp_(UtilAll::currentTimeMillis()),
      request_msg_(nullptr),
      timeout_millis_(timeoutMillis),
      count_down_latch_(nullptr),
      response_msg_(nullptr),
      send_request_ok_(false),
      cause_(nullptr) {
  if (nullptr == requestCallback) {
    count_down_latch_.reset(new latch(1));
  }
}

void RequestResponseFuture::executeRequestCallback() noexcept {
  if (request_callback_ != nullptr) {
    if (send_request_ok_ && cause_ == nullptr) {
      try {
        request_callback_->invokeOnSuccess(std::move(response_msg_));
      } catch (const std::exception& e) {
        LOG_WARN_NEW("RequestCallback throw an exception: {}", e.what());
      }
    } else {
      try {
        std::rethrow_exception(cause_);
      } catch (MQException& e) {
        request_callback_->invokeOnException(e);
      } catch (const std::exception& e) {
        LOG_WARN_NEW("unexpected exception in RequestResponseFuture: {}", e.what());
      }
    }
  }
}

bool RequestResponseFuture::isTimeout() {
  auto diff = UtilAll::currentTimeMillis() - begin_timestamp_;
  return diff > timeout_millis_;
}

MessagePtr RequestResponseFuture::waitResponseMessage(int64_t timeout) {
  if (count_down_latch_ != nullptr) {
    if (timeout < 0) {
      timeout = 0;
    }
    count_down_latch_->wait(timeout, time_unit::milliseconds);
  }
  return response_msg_;
}

void RequestResponseFuture::putResponseMessage(MessagePtr responseMsg) {
  response_msg_ = std::move(responseMsg);
  if (count_down_latch_ != nullptr) {
    count_down_latch_->count_down();
  }
}

}  // namespace rocketmq
