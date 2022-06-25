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

#ifndef __TRANSACTIONMQPRODUCERIMPL_H__
#define __TRANSACTIONMQPRODUCERIMPL_H__

#include <boost/asio.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/smart_ptr.hpp>
#include <memory>
#include <string>
#include "DefaultMQProducerImpl.h"
#include "MQMessageExt.h"
#include "TransactionListener.h"
#include "TransactionSendResult.h"

namespace rocketmq {

class TransactionMQProducerImpl : public DefaultMQProducerImpl {
 public:
  TransactionMQProducerImpl(const std::string& producerGroup)
      : DefaultMQProducerImpl(producerGroup), m_thread_num(1), m_ioServiceWork(m_ioService) {}
  virtual ~TransactionMQProducerImpl() {}
  void start();
  void shutdown();
  std::shared_ptr<TransactionListener> getTransactionListener() { return m_transactionListener; }
  void setTransactionListener(TransactionListener* listener) { m_transactionListener.reset(listener); }
  TransactionSendResult sendMessageInTransaction(MQMessage& msg, void* arg);
  void checkTransactionState(const std::string& addr,
                             const MQMessageExt& message,
                             long tranStateTableOffset,
                             long commitLogOffset,
                             const std::string& msgId,
                             const std::string& transactionId,
                             const std::string& offsetMsgId);

 private:
  void initTransactionEnv();
  void destroyTransactionEnv();
  void endTransaction(SendResult& sendResult, LocalTransactionState& localTransactionState);
  void checkTransactionStateImpl(const std::string& addr,
                                 const MQMessageExt& message,
                                 long tranStateTableOffset,
                                 long commitLogOffset,
                                 const std::string& msgId,
                                 const std::string& transactionId,
                                 const std::string& offsetMsgId);

 private:
  std::shared_ptr<TransactionListener> m_transactionListener;
  int m_thread_num;
  boost::thread_group m_threadpool;
  boost::asio::io_service m_ioService;
  boost::asio::io_service::work m_ioServiceWork;
};
}  // namespace rocketmq

#endif
