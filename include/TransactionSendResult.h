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

#ifndef __TRANSACTIONSENDRESULT_H__
#define __TRANSACTIONSENDRESULT_H__

#include "SendResult.h"

namespace rocketmq {

enum LocalTransactionState { COMMIT_MESSAGE, ROLLBACK_MESSAGE, UNKNOWN };

class ROCKETMQCLIENT_API TransactionSendResult : public SendResult {
 public:
  TransactionSendResult() {}

  TransactionSendResult(const SendStatus& sendStatus,
                        const std::string& msgId,
                        const std::string& offsetMsgId,
                        const MQMessageQueue& messageQueue,
                        int64 queueOffset)
      : SendResult(sendStatus, msgId, offsetMsgId, messageQueue, queueOffset) {}

  LocalTransactionState getLocalTransactionState() { return m_localTransactionState; }

  void setLocalTransactionState(LocalTransactionState localTransactionState) {
    m_localTransactionState = localTransactionState;
  }

 private:
  LocalTransactionState m_localTransactionState;
};
}  // namespace rocketmq
#endif