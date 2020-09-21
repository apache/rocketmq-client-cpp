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
#ifndef ROCKET_COMMON_PULLCALLBACKWRAP_H_
#define ROCKET_COMMON_PULLCALLBACKWRAP_H_

#include "InvokeCallback.h"
#include "MQClientAPIImpl.h"
#include "PullCallback.h"
#include "ResponseFuture.h"

namespace rocketmq {

class PullCallbackWrap : public InvokeCallback {
 public:
  PullCallbackWrap(PullCallback* pullCallback, MQClientAPIImpl* pClientAPI);

  void operationComplete(ResponseFuture* responseFuture) noexcept override;

 private:
  PullCallback* pull_callback_;
  MQClientAPIImpl* client_api_impl_;
};

}  // namespace rocketmq

#endif  // ROCKET_COMMON_PULLCALLBACKWRAP_H_
