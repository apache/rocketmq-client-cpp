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
#include "PullCallbackWrap.h"

namespace rocketmq {

PullCallbackWrap::PullCallbackWrap(PullCallback* pullCallback, MQClientAPIImpl* pClientAPI)
    : m_pullCallback(pullCallback), m_pClientAPI(pClientAPI) {}

void PullCallbackWrap::operationComplete(ResponseFuture* responseFuture) noexcept {
  std::unique_ptr<RemotingCommand> response(responseFuture->getResponseCommand());  // avoid RemotingCommand leak

  if (m_pullCallback == nullptr) {
    LOG_ERROR("m_pullCallback is NULL, AsyncPull could not continue");
    return;
  }

  if (response != nullptr) {
    try {
      std::unique_ptr<PullResult> pullResult(m_pClientAPI->processPullResponse(response.get()));
      assert(pullResult != nullptr);
      m_pullCallback->onSuccess(*pullResult);
    } catch (MQException& e) {
      m_pullCallback->onException(e);
    }
  } else {
    std::string err;
    if (!responseFuture->isSendRequestOK()) {
      err = "send request failed";
    } else if (responseFuture->isTimeout()) {
      err = "wait response timeout";
    } else {
      err = "unknown reason";
    }
    MQException exception(err, -1, __FILE__, __LINE__);
    m_pullCallback->onException(exception);
  }

  // auto delete callback
  if (m_pullCallback->getPullCallbackType() == PULL_CALLBACK_TYPE_AUTO_DELETE) {
    deleteAndZero(m_pullCallback);
  }
}

}  // namespace rocketmq
