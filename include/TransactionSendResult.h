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
#ifndef ROCKETMQ_TRANSACTIONSENDRESULT_H_
#define ROCKETMQ_TRANSACTIONSENDRESULT_H_

#include "SendResult.h"

namespace rocketmq {

enum LocalTransactionState { COMMIT_MESSAGE, ROLLBACK_MESSAGE, UNKNOWN };

class ROCKETMQCLIENT_API TransactionSendResult : public SendResult {
 public:
  TransactionSendResult(const SendResult& sendResult) : SendResult(sendResult), local_transaction_state_(UNKNOWN) {}

  inline LocalTransactionState local_transaction_state() const { return local_transaction_state_; }

  inline void set_local_transaction_state(LocalTransactionState localTransactionState) {
    local_transaction_state_ = localTransactionState;
  }

 private:
  LocalTransactionState local_transaction_state_;
};

}  // namespace rocketmq

#endif  // ROCKETMQ_TRANSACTIONSENDRESULT_H_
