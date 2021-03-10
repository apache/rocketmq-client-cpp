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
#ifndef ROCKETMQ_TRANSPORT_RESPONSEFUTURE_H_
#define ROCKETMQ_TRANSPORT_RESPONSEFUTURE_H_

#include <memory>
#include "InvokeCallback.h"
#include "RemotingCommand.h"
#include "concurrent/latch.hpp"

namespace rocketmq {

class ResponseFuture;
typedef std::shared_ptr<ResponseFuture> ResponseFuturePtr;

class ResponseFuture {
 public:
  ResponseFuture(int requestCode,
                 int opaque,
                 int64_t timeoutMillis,
                 std::unique_ptr<InvokeCallback> invokeCallback = nullptr);
  virtual ~ResponseFuture();

  void releaseThreadCondition();

  bool hasInvokeCallback();

  void executeInvokeCallback() noexcept;

  // for sync request
  std::unique_ptr<RemotingCommand> waitResponse(int timeoutMillis);
  void putResponse(std::unique_ptr<RemotingCommand> responseCommand);

  // for async request
  std::unique_ptr<RemotingCommand> getResponseCommand();
  void setResponseCommand(std::unique_ptr<RemotingCommand> responseCommand);

  bool isTimeout() const;
  int64_t leftTime() const;

 public:
  inline int request_code() const { return request_code_; }
  inline int opaque() const { return opaque_; }
  inline int64_t timeout_millis() const { return timeout_millis_; }

  inline int64_t begin_timestamp() const { return begin_timestamp_; }

  inline bool send_request_ok() const { return send_request_ok_; }
  inline void set_send_request_ok(bool sendRequestOK = true) { send_request_ok_ = sendRequestOK; };

  inline std::unique_ptr<InvokeCallback>& invoke_callback() { return invoke_callback_; }

 private:
  int request_code_;
  int opaque_;
  int64_t timeout_millis_;
  std::unique_ptr<InvokeCallback> invoke_callback_;

  int64_t begin_timestamp_;
  bool send_request_ok_;

  std::unique_ptr<RemotingCommand> response_command_;

  std::unique_ptr<latch> count_down_latch_;  // use for synchronization rpc
};

}  // namespace rocketmq

#endif  // ROCKETMQ_TRANSPORT_RESPONSEFUTURE_H_
