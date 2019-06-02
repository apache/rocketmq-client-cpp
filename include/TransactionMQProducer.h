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

#ifndef __TRANSACTIONMQPRODUCER_H__
#define __TRANSACTIONMQPRODUCER_H__

#include <memory>
#include <string>
#include "DefaultMQProducer.h"
#include "MQMessageExt.h"
#include "TransactionListener.h"
#include "TransactionSendResult.h"

namespace rocketmq {

class ROCKETMQCLIENT_API TransactionMQProducer : public DefaultMQProducer {
 public:
  TransactionMQProducer(const std::string& producerGroup) : DefaultMQProducer(producerGroup) {}
  virtual ~TransactionMQProducer() {}
  void start();
  void shutdown();
  std::shared_ptr<TransactionListener> getCheckListener() { return m_transactionListener; }
  void setTransactionListener(TransactionListener* listener) { m_transactionListener.reset(listener); }
  TransactionSendResult sendMessageInTransaction(MQMessage& msg, void* arg);
  void checkTransactionState(const std::string& addr, const MQMessageExt& message,
                             long m_tranStateTableOffset,
                             long m_commitLogOffset,
                             std::string m_msgId,
                             std::string m_transactionId,
                             std::string m_offsetMsgId);

 private:
  void initTransactionEnv();
  void destroyTransactionEnv();
  void endTransaction(SendResult& sendResult,
                      LocalTransactionState& localTransactionState);

 private:
  std::shared_ptr<TransactionListener> m_transactionListener;
};
}  // namespace rocketmq

#endif
