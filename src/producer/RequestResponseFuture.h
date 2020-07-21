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
#ifndef ROCKETMQ_PRODUCER_REQUESTRESPONSEFUTURE_H_
#define ROCKETMQ_PRODUCER_REQUESTRESPONSEFUTURE_H_

#include <exception>
#include <string>

#include "Message.h"
#include "RequestCallback.h"
#include "concurrent/latch.hpp"

namespace rocketmq {

class RequestResponseFuture {
 public:
  RequestResponseFuture(const std::string& correlationId, long timeoutMillis, RequestCallback* requestCallback);

  void executeRequestCallback() noexcept;

  bool isTimeout();

  MessagePtr waitResponseMessage(int64_t timeout);
  void putResponseMessage(MessagePtr responseMsg);

 public:
  inline const std::string& correlation_id() const { return correlation_id_; }

  inline bool send_request_ok() const { return send_request_ok_; }
  inline void set_send_request_ok(bool sendRequestOk) { send_request_ok_ = sendRequestOk; }

  inline std::exception_ptr cause() const { return cause_; }
  inline void set_cause(std::exception_ptr cause) { cause_ = cause; }

 private:
  std::string correlation_id_;
  RequestCallback* request_callback_;
  uint64_t begin_timestamp_;
  MessagePtr request_msg_;
  long timeout_millis_;
  std::unique_ptr<latch> count_down_latch_;  // use for synchronization rpc
  MessagePtr response_msg_;
  bool send_request_ok_;
  std::exception_ptr cause_;
};

}  // namespace rocketmq

#endif  // ROCKETMQ_PRODUCER_REQUESTRESPONSEFUTURE_H_
