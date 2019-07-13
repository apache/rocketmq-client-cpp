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

#include "TransactionMQProducer.h"
#include <string>
#include "CommandHeader.h"
#include "Logging.h"
#include "MQClientFactory.h"
#include "MQDecoder.h"
#include "MessageSysFlag.h"
#include "TransactionListener.h"
#include "TransactionSendResult.h"

using namespace std;
namespace rocketmq {

void TransactionMQProducer::initTransactionEnv() {}

void TransactionMQProducer::destroyTransactionEnv() {}

TransactionSendResult TransactionMQProducer::sendMessageInTransaction(MQMessage& msg, void* arg) {
  if (nullptr == m_transactionListener) {
    THROW_MQEXCEPTION(MQClientException, "transactionListener is null", -1);
  }

  SendResult sendResult;
  msg.setProperty(MQMessage::PROPERTY_TRANSACTION_PREPARED, "true");
  msg.setProperty(MQMessage::PROPERTY_PRODUCER_GROUP, getGroupName());
  try {
    sendResult = send(msg);
  } catch (MQException& e) {
    THROW_MQEXCEPTION(MQClientException, e.what(), -1);
  }

  LOG_DEBUG("sendMessageInTransaction result:%s", sendResult.toString().data());
  LocalTransactionState localTransactionState = LocalTransactionState::UNKNOW;
  switch (sendResult.getSendStatus()) {
    case SendStatus::SEND_OK:
      try {
        if (sendResult.getTransactionId() != "") {
          msg.setProperty("__transactionId__", sendResult.getTransactionId());
        }
        string transactionId = msg.getProperty(MQMessage::PROPERTY_UNIQ_CLIENT_MESSAGE_ID_KEYIDX);
        if (transactionId != "") {
          msg.setTransactionId(transactionId);
        }
        LOG_DEBUG("sendMessageInTransaction, msgId:%s, transactionId:%s",
                  sendResult.getMsgId().data(), transactionId.data());
        localTransactionState = m_transactionListener->executeLocalTransaction(msg, arg);
        if (localTransactionState != LocalTransactionState::COMMIT_MESSAGE) {
          LOG_WARN("executeLocalTransaction ret not LocalTransactionState::commit, msg:%s", msg.toString().data());
        }
      } catch (MQException& e) {
        THROW_MQEXCEPTION(MQClientException, e.what(), -1);
      }
      break;
    case SendStatus::SEND_FLUSH_DISK_TIMEOUT:
    case SendStatus::SEND_FLUSH_SLAVE_TIMEOUT:
    case SendStatus::SEND_SLAVE_NOT_AVAILABLE:
      localTransactionState = LocalTransactionState::ROLLBACK_MESSAGE;
      break;
    default:
      break;
  }

  try {
    endTransaction(sendResult, localTransactionState);
  } catch (MQException& e) {
    LOG_WARN("endTransaction exception:%s", e.what());
  }

  TransactionSendResult transactionSendResult(sendResult.getSendStatus(), sendResult.getMsgId(),
                                              sendResult.getOffsetMsgId(), sendResult.getMessageQueue(),
                                              sendResult.getQueueOffset());
  transactionSendResult.setTransactionId(msg.getTransactionId());
  transactionSendResult.setLocalTransactionState(localTransactionState);
  return transactionSendResult;
}

void TransactionMQProducer::endTransaction(SendResult& sendResult, LocalTransactionState& localTransactionState) {
  MQMessageId id;
  if (sendResult.getOffsetMsgId() != "") {
    id = MQDecoder::decodeMessageId(sendResult.getOffsetMsgId());
  } else {
    id = MQDecoder::decodeMessageId(sendResult.getMsgId());
  }
  string transId = sendResult.getTransactionId();

  int commitOrRollback = 0;
  switch (localTransactionState) {
    case COMMIT_MESSAGE:
      commitOrRollback = MessageSysFlag::TransactionCommitType;
      break;
    case ROLLBACK_MESSAGE:
      commitOrRollback = MessageSysFlag::TransactionRollbackType;
      break;
    case UNKNOW:
      commitOrRollback = MessageSysFlag::TransactionNotType;
      break;
    default:
      commitOrRollback = MessageSysFlag::TransactionNotType;
      break;
  }

  bool fromTransCheck = false;
  EndTransactionRequestHeader* requestHeader =
      new EndTransactionRequestHeader(getGroupName(), sendResult.getQueueOffset(), id.getOffset(), commitOrRollback,
                                      fromTransCheck, sendResult.getMsgId(), transId);
  LOG_DEBUG("endTransaction: msg:%s", requestHeader->toString().data());
  getFactory()->endTransactionOneway(sendResult.getMessageQueue(), requestHeader, getSessionCredentials());
}

void TransactionMQProducer::checkTransactionState(const std::string& addr, const MQMessageExt& message,
                                                  long tranStateTableOffset,
                                                  long commitLogOffset,
                                                  const std::string& msgId,
                                                  const std::string& transactionId,
                                                  const std::string& offsetMsgId) {
  LocalTransactionState localTransactionState = UNKNOW;

  try {
    m_transactionListener->checkLocalTransaction(message);
  } catch (MQException& e) {
    LOG_INFO("checkTransactionState, checkLocalTransaction exception: %s", e.what());
  }
  
  EndTransactionRequestHeader* endHeader = new EndTransactionRequestHeader();
  endHeader->m_commitLogOffset = commitLogOffset;
  endHeader->m_producerGroup = getGroupName();
  endHeader->m_tranStateTableOffset = tranStateTableOffset;
  endHeader->m_fromTransactionCheck = true;

  string uniqueKey = transactionId;
  if (transactionId.empty()) {
    uniqueKey = message.getMsgId();
  }
  
  endHeader->m_msgId = uniqueKey;
  endHeader->m_transactionId = transactionId;
  switch (localTransactionState) {
    case COMMIT_MESSAGE:
      endHeader->m_commitOrRollback = MessageSysFlag::TransactionCommitType;
      break;
    case ROLLBACK_MESSAGE:
      endHeader->m_commitOrRollback = MessageSysFlag::TransactionRollbackType;
      LOG_WARN("when broker check, client rollback this transaction, %s", endHeader->toString().data());
      break;
    case UNKNOW:
      endHeader->m_commitOrRollback = MessageSysFlag::TransactionNotType;
      LOG_WARN("when broker check, client does not know this transaction state, %s", endHeader->toString().data());
      break;
    default:
      break;
  }

  LOG_INFO("checkTransactionState, endTransactionOneway: uniqueKey:%s, client state:%d, end header: %s", uniqueKey.data(), localTransactionState,
    endHeader->toString().data());

  string remark;
  try {
    getFactory()->getMQClientAPIImpl()->endTransactionOneway(addr, endHeader, remark, getSessionCredentials());
  } catch (MQException& e) {
    LOG_ERROR("endTransactionOneway exception:%s", e.what());
    throw e;
  }
}

void TransactionMQProducer::start() {
  initTransactionEnv();
  DefaultMQProducer::start();
}

void TransactionMQProducer::shutdown() {
  DefaultMQProducer::shutdown();
  destroyTransactionEnv();
}

}  // namespace rocketmq
