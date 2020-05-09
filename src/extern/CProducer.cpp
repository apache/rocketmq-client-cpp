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
#include "c/CProducer.h"

#include <cstring>
#include <functional>
#include <typeindex>

#include "ClientRPCHook.h"
#include "DefaultMQProducer.h"
#include "Logging.h"
#include "MQClientErrorContainer.h"
#include "TransactionMQProducer.h"
#include "UtilAll.h"

using namespace rocketmq;

class LocalTransactionExecutorInner {
 public:
  LocalTransactionExecutorInner(CLocalTransactionExecutorCallback callback, CMessage* message, void* userData)
      : m_excutorCallback(callback), m_message(message), m_userData(userData) {}

  ~LocalTransactionExecutorInner() = default;

 public:
  CLocalTransactionExecutorCallback m_excutorCallback;
  CMessage* m_message;
  void* m_userData;
};

class LocalTransactionListenerInner : public TransactionListener {
 public:
  LocalTransactionListenerInner(CProducer* producer, CLocalTransactionCheckerCallback callback, void* userData)
      : m_producer(producer), m_checkerCallback(callback), m_userData(userData) {}

  ~LocalTransactionListenerInner() = default;

  LocalTransactionState executeLocalTransaction(const MQMessage& message, void* arg) override {
    if (m_checkerCallback == nullptr) {
      return LocalTransactionState::UNKNOWN;
    }
    auto* msg = reinterpret_cast<CMessage*>(const_cast<MQMessage*>(&message));
    auto* executorInner = reinterpret_cast<LocalTransactionExecutorInner*>(arg);
    auto status = executorInner->m_excutorCallback(m_producer, msg, executorInner->m_userData);
    switch (status) {
      case E_COMMIT_TRANSACTION:
        return LocalTransactionState::COMMIT_MESSAGE;
      case E_ROLLBACK_TRANSACTION:
        return LocalTransactionState::ROLLBACK_MESSAGE;
      default:
        return LocalTransactionState::UNKNOWN;
    }
  }

  LocalTransactionState checkLocalTransaction(const MQMessageExt& message) override {
    if (m_checkerCallback == NULL) {
      return LocalTransactionState::UNKNOWN;
    }
    auto* msgExt = reinterpret_cast<CMessageExt*>(const_cast<MQMessageExt*>(&message));
    auto status = m_checkerCallback(m_producer, msgExt, m_userData);
    switch (status) {
      case E_COMMIT_TRANSACTION:
        return LocalTransactionState::COMMIT_MESSAGE;
      case E_ROLLBACK_TRANSACTION:
        return LocalTransactionState::ROLLBACK_MESSAGE;
      default:
        return LocalTransactionState::UNKNOWN;
    }
  }

 private:
  CProducer* m_producer;
  CLocalTransactionCheckerCallback m_checkerCallback;
  void* m_userData;
};

class SelectMessageQueueInner : public MessageQueueSelector {
 public:
  MQMessageQueue select(const std::vector<MQMessageQueue>& mqs, const MQMessage& msg, void* arg) override {
    std::string shardingKey = UtilAll::to_string((char*)arg);
    auto index = std::hash<std::string>{}(shardingKey) % mqs.size();
    return mqs[index % mqs.size()];
  }
};

class SelectMessageQueue : public MessageQueueSelector {
 public:
  SelectMessageQueue(QueueSelectorCallback callback) { m_callback = callback; }

  MQMessageQueue select(const std::vector<MQMessageQueue>& mqs, const MQMessage& msg, void* arg) override {
    auto* message = reinterpret_cast<CMessage*>(const_cast<MQMessage*>(&msg));
    // Get the index of sending MQMessageQueue through callback function.
    auto index = m_callback(mqs.size(), message, arg);
    return mqs[index];
  }

 private:
  QueueSelectorCallback m_callback;
};

class COnSendCallback : public AutoDeleteSendCallback {
 public:
  COnSendCallback(COnSendSuccessCallback sendSuccessCallback,
                  COnSendExceptionCallback sendExceptionCallback,
                  CMessage* message,
                  void* userData)
      : m_sendSuccessCallback(sendSuccessCallback),
        m_sendExceptionCallback(sendExceptionCallback),
        m_message(message),
        m_userData(userData) {}

  virtual ~COnSendCallback() = default;

  void onSuccess(SendResult& sendResult) override {
    CSendResult result;
    result.sendStatus = CSendStatus((int)sendResult.getSendStatus());
    result.offset = sendResult.getQueueOffset();
    strncpy(result.msgId, sendResult.getMsgId().c_str(), MAX_MESSAGE_ID_LENGTH - 1);
    result.msgId[MAX_MESSAGE_ID_LENGTH - 1] = 0;
    m_sendSuccessCallback(result, m_message, m_userData);
  }

  void onException(MQException& e) noexcept override {
    CMQException exception;
    exception.error = e.GetError();
    exception.line = e.GetLine();
    strncpy(exception.msg, e.what(), MAX_EXEPTION_MSG_LENGTH - 1);
    strncpy(exception.file, e.GetFile(), MAX_EXEPTION_FILE_LENGTH - 1);
    m_sendExceptionCallback(exception, m_message, m_userData);
  }

 private:
  COnSendSuccessCallback m_sendSuccessCallback;
  COnSendExceptionCallback m_sendExceptionCallback;
  CMessage* m_message;
  void* m_userData;
};

class CSendCallback : public AutoDeleteSendCallback {
 public:
  CSendCallback(CSendSuccessCallback sendSuccessCallback, CSendExceptionCallback sendExceptionCallback)
      : m_sendSuccessCallback(sendSuccessCallback), m_sendExceptionCallback(sendExceptionCallback) {}

  virtual ~CSendCallback() = default;

  void onSuccess(SendResult& sendResult) override {
    CSendResult result;
    result.sendStatus = CSendStatus((int)sendResult.getSendStatus());
    result.offset = sendResult.getQueueOffset();
    strncpy(result.msgId, sendResult.getMsgId().c_str(), MAX_MESSAGE_ID_LENGTH - 1);
    result.msgId[MAX_MESSAGE_ID_LENGTH - 1] = 0;
    m_sendSuccessCallback(result);
  }

  void onException(MQException& e) noexcept override {
    CMQException exception;
    exception.error = e.GetError();
    exception.line = e.GetLine();
    strncpy(exception.msg, e.what(), MAX_EXEPTION_MSG_LENGTH - 1);
    strncpy(exception.file, e.GetFile(), MAX_EXEPTION_FILE_LENGTH - 1);
    m_sendExceptionCallback(exception);
  }

 private:
  CSendSuccessCallback m_sendSuccessCallback;
  CSendExceptionCallback m_sendExceptionCallback;
};

CProducer* CreateProducer(const char* groupId) {
  if (groupId == NULL) {
    return NULL;
  }
  auto* defaultMQProducer = new DefaultMQProducer(groupId);
  return reinterpret_cast<CProducer*>(defaultMQProducer);
}

CProducer* CreateOrderlyProducer(const char* groupId) {
  return CreateProducer(groupId);
}

CProducer* CreateTransactionProducer(const char* groupId, CLocalTransactionCheckerCallback callback, void* userData) {
  if (groupId == NULL) {
    return NULL;
  }
  auto* transactionMQProducer = new TransactionMQProducer(groupId);
  auto* producer = reinterpret_cast<CProducer*>(static_cast<DefaultMQProducer*>(transactionMQProducer));
  auto* transcationListener = new LocalTransactionListenerInner(producer, callback, userData);
  transactionMQProducer->setTransactionListener(transcationListener);
  return producer;
}

int DestroyProducer(CProducer* producer) {
  if (producer == nullptr) {
    return NULL_POINTER;
  }
  delete reinterpret_cast<DefaultMQProducer*>(producer);
  return OK;
}

int StartProducer(CProducer* producer) {
  if (producer == NULL) {
    return NULL_POINTER;
  }
  try {
    reinterpret_cast<DefaultMQProducer*>(producer)->start();
  } catch (std::exception& e) {
    MQClientErrorContainer::setErr(std::string(e.what()));
    return PRODUCER_START_FAILED;
  }
  return OK;
}

int ShutdownProducer(CProducer* producer) {
  if (producer == NULL) {
    return NULL_POINTER;
  }
  reinterpret_cast<DefaultMQProducer*>(producer)->shutdown();
  return OK;
}

int SetProducerNameServerAddress(CProducer* producer, const char* namesrv) {
  if (producer == NULL) {
    return NULL_POINTER;
  }
  reinterpret_cast<DefaultMQProducer*>(producer)->setNamesrvAddr(namesrv);
  return OK;
}

// Deprecated
int SetProducerNameServerDomain(CProducer* producer, const char* domain) {
  if (producer == NULL) {
    return NULL_POINTER;
  }
  // reinterpret_cast<DefaultMQProducer*>(producer)->setNamesrvDomain(domain);
  return OK;
}

int SendMessageSync(CProducer* producer, CMessage* msg, CSendResult* result) {
  if (producer == NULL || msg == NULL || result == NULL) {
    return NULL_POINTER;
  }
  try {
    auto* defaultMQProducer = reinterpret_cast<DefaultMQProducer*>(producer);
    auto* message = reinterpret_cast<MQMessage*>(msg);
    auto sendResult = defaultMQProducer->send(message);
    switch (sendResult.getSendStatus()) {
      case SEND_OK:
        result->sendStatus = E_SEND_OK;
        break;
      case SEND_FLUSH_DISK_TIMEOUT:
        result->sendStatus = E_SEND_FLUSH_DISK_TIMEOUT;
        break;
      case SEND_FLUSH_SLAVE_TIMEOUT:
        result->sendStatus = E_SEND_FLUSH_SLAVE_TIMEOUT;
        break;
      case SEND_SLAVE_NOT_AVAILABLE:
        result->sendStatus = E_SEND_SLAVE_NOT_AVAILABLE;
        break;
      default:
        result->sendStatus = E_SEND_OK;
        break;
    }
    result->offset = sendResult.getQueueOffset();
    strncpy(result->msgId, sendResult.getMsgId().c_str(), MAX_MESSAGE_ID_LENGTH - 1);
    result->msgId[MAX_MESSAGE_ID_LENGTH - 1] = 0;
  } catch (std::exception& e) {
    MQClientErrorContainer::setErr(std::string(e.what()));
    return PRODUCER_SEND_SYNC_FAILED;
  }
  return OK;
}

int SendBatchMessage(CProducer* producer, CBatchMessage* batcMsg, CSendResult* result) {
  if (producer == NULL || batcMsg == NULL || result == NULL) {
    return NULL_POINTER;
  }
  try {
    auto* defaultMQProducer = reinterpret_cast<DefaultMQProducer*>(producer);
    auto* message = reinterpret_cast<std::vector<MQMessage*>*>(batcMsg);
    auto sendResult = defaultMQProducer->send(*message);
    switch (sendResult.getSendStatus()) {
      case SEND_OK:
        result->sendStatus = E_SEND_OK;
        break;
      case SEND_FLUSH_DISK_TIMEOUT:
        result->sendStatus = E_SEND_FLUSH_DISK_TIMEOUT;
        break;
      case SEND_FLUSH_SLAVE_TIMEOUT:
        result->sendStatus = E_SEND_FLUSH_SLAVE_TIMEOUT;
        break;
      case SEND_SLAVE_NOT_AVAILABLE:
        result->sendStatus = E_SEND_SLAVE_NOT_AVAILABLE;
        break;
      default:
        result->sendStatus = E_SEND_OK;
        break;
    }
    result->offset = sendResult.getQueueOffset();
    strncpy(result->msgId, sendResult.getMsgId().c_str(), MAX_MESSAGE_ID_LENGTH - 1);
    result->msgId[MAX_MESSAGE_ID_LENGTH - 1] = 0;
  } catch (std::exception& e) {
    return PRODUCER_SEND_SYNC_FAILED;
  }
  return OK;
}

int SendMessageAsync(CProducer* producer,
                     CMessage* msg,
                     CSendSuccessCallback sendSuccessCallback,
                     CSendExceptionCallback sendExceptionCallback) {
  if (producer == NULL || msg == NULL || sendSuccessCallback == NULL || sendExceptionCallback == NULL) {
    return NULL_POINTER;
  }
  auto* defaultMQProducer = reinterpret_cast<DefaultMQProducer*>(producer);
  auto* message = reinterpret_cast<MQMessage*>(msg);
  auto* sendCallback = new CSendCallback(sendSuccessCallback, sendExceptionCallback);
  defaultMQProducer->send(message, sendCallback);
  return OK;
}

int SendAsync(CProducer* producer,
              CMessage* msg,
              COnSendSuccessCallback sendSuccessCallback,
              COnSendExceptionCallback sendExceptionCallback,
              void* userData) {
  if (producer == NULL || msg == NULL || sendSuccessCallback == NULL || sendExceptionCallback == NULL) {
    return NULL_POINTER;
  }
  auto* defaultMQProducer = reinterpret_cast<DefaultMQProducer*>(producer);
  auto* message = reinterpret_cast<MQMessage*>(msg);
  auto* sendCallback = new COnSendCallback(sendSuccessCallback, sendExceptionCallback, msg, userData);
  defaultMQProducer->send(message, sendCallback);
  return OK;
}

int SendMessageOneway(CProducer* producer, CMessage* msg) {
  if (producer == NULL || msg == NULL) {
    return NULL_POINTER;
  }
  auto* defaultMQProducer = reinterpret_cast<DefaultMQProducer*>(producer);
  auto* message = reinterpret_cast<MQMessage*>(msg);
  try {
    defaultMQProducer->sendOneway(message);
  } catch (std::exception& e) {
    return PRODUCER_SEND_ONEWAY_FAILED;
  }
  return OK;
}

int SendMessageOnewayOrderly(CProducer* producer, CMessage* msg, QueueSelectorCallback selector, void* arg) {
  if (producer == NULL || msg == NULL) {
    return NULL_POINTER;
  }
  auto* defaultMQProducer = reinterpret_cast<DefaultMQProducer*>(producer);
  auto* message = reinterpret_cast<MQMessage*>(msg);
  try {
    SelectMessageQueue selectMessageQueue(selector);
    defaultMQProducer->sendOneway(message, &selectMessageQueue, arg);
  } catch (std::exception& e) {
    MQClientErrorContainer::setErr(std::string(e.what()));
    return PRODUCER_SEND_ONEWAY_FAILED;
  }
  return OK;
}

int SendMessageOrderlyAsync(CProducer* producer,
                            CMessage* msg,
                            QueueSelectorCallback selectorCallback,
                            void* arg,
                            CSendSuccessCallback sendSuccessCallback,
                            CSendExceptionCallback sendExceptionCallback) {
  if (producer == NULL || msg == NULL || selectorCallback == NULL || sendSuccessCallback == NULL ||
      sendExceptionCallback == NULL) {
    return NULL_POINTER;
  }
  auto* defaultMQProducer = reinterpret_cast<DefaultMQProducer*>(producer);
  auto* message = reinterpret_cast<MQMessage*>(msg);
  auto* cSendCallback = new CSendCallback(sendSuccessCallback, sendExceptionCallback);
  // Constructing SelectMessageQueue objects through function pointer callback
  SelectMessageQueue selectMessageQueue(selectorCallback);
  defaultMQProducer->send(message, &selectMessageQueue, arg, cSendCallback);
  return OK;
}

int SendMessageOrderly(CProducer* producer,
                       CMessage* msg,
                       QueueSelectorCallback selectorCallback,
                       void* arg,
                       int autoRetryTimes,
                       CSendResult* result) {
  if (producer == NULL || msg == NULL || selectorCallback == NULL || arg == NULL || result == NULL) {
    return NULL_POINTER;
  }
  auto* defaultMQProducer = reinterpret_cast<DefaultMQProducer*>(producer);
  auto* message = reinterpret_cast<MQMessage*>(msg);
  try {
    // Constructing SelectMessageQueue objects through function pointer callback
    SelectMessageQueue selectMessageQueue(selectorCallback);
    SendResult sendResult = defaultMQProducer->send(message, &selectMessageQueue, arg);
    // Convert SendStatus to CSendStatus
    result->sendStatus = CSendStatus((int)sendResult.getSendStatus());
    result->offset = sendResult.getQueueOffset();
    strncpy(result->msgId, sendResult.getMsgId().c_str(), MAX_MESSAGE_ID_LENGTH - 1);
    result->msgId[MAX_MESSAGE_ID_LENGTH - 1] = 0;
  } catch (std::exception& e) {
    MQClientErrorContainer::setErr(std::string(e.what()));
    return PRODUCER_SEND_ORDERLY_FAILED;
  }
  return OK;
}

int SendMessageOrderlyByShardingKey(CProducer* producer, CMessage* msg, const char* shardingKey, CSendResult* result) {
  if (producer == NULL || msg == NULL || shardingKey == NULL || result == NULL) {
    return NULL_POINTER;
  }
  auto* defaultMQProducer = reinterpret_cast<DefaultMQProducer*>(producer);
  auto* message = reinterpret_cast<MQMessage*>(msg);
  try {
    // Constructing SelectMessageQueue objects through function pointer callback
    int retryTimes = 3;
    SelectMessageQueueInner selectMessageQueue;
    SendResult sendResult = defaultMQProducer->send(message, &selectMessageQueue, (void*)shardingKey, retryTimes);
    // Convert SendStatus to CSendStatus
    result->sendStatus = CSendStatus((int)sendResult.getSendStatus());
    result->offset = sendResult.getQueueOffset();
    strncpy(result->msgId, sendResult.getMsgId().c_str(), MAX_MESSAGE_ID_LENGTH - 1);
    result->msgId[MAX_MESSAGE_ID_LENGTH - 1] = 0;
  } catch (std::exception& e) {
    MQClientErrorContainer::setErr(std::string(e.what()));
    return PRODUCER_SEND_ORDERLY_FAILED;
  }
  return OK;
}

int SendMessageTransaction(CProducer* producer,
                           CMessage* msg,
                           CLocalTransactionExecutorCallback callback,
                           void* userData,
                           CSendResult* result) {
  if (producer == NULL || msg == NULL || callback == NULL || result == NULL) {
    return NULL_POINTER;
  }
  try {
    auto* transactionMQProducer = reinterpret_cast<DefaultMQProducer*>(producer);
    auto* message = reinterpret_cast<MQMessage*>(msg);
    LocalTransactionExecutorInner executorInner(callback, msg, userData);
    auto sendResult = transactionMQProducer->sendMessageInTransaction(message, &executorInner);
    result->sendStatus = CSendStatus((int)sendResult.getSendStatus());
    result->offset = sendResult.getQueueOffset();
    strncpy(result->msgId, sendResult.getMsgId().c_str(), MAX_MESSAGE_ID_LENGTH - 1);
    result->msgId[MAX_MESSAGE_ID_LENGTH - 1] = 0;
  } catch (std::exception& e) {
    MQClientErrorContainer::setErr(std::string(e.what()));
    return PRODUCER_SEND_TRANSACTION_FAILED;
  }
  return OK;
}

int SetProducerGroupName(CProducer* producer, const char* groupName) {
  if (producer == NULL) {
    return NULL_POINTER;
  }
  reinterpret_cast<DefaultMQProducer*>(producer)->setGroupName(groupName);
  return OK;
}

int SetProducerInstanceName(CProducer* producer, const char* instanceName) {
  if (producer == NULL) {
    return NULL_POINTER;
  }
  reinterpret_cast<DefaultMQProducer*>(producer)->setInstanceName(instanceName);
  return OK;
}

int SetProducerSessionCredentials(CProducer* producer,
                                  const char* accessKey,
                                  const char* secretKey,
                                  const char* onsChannel) {
  if (producer == NULL) {
    return NULL_POINTER;
  }
  auto rpcHook = std::make_shared<ClientRPCHook>(SessionCredentials(accessKey, secretKey, onsChannel));
  reinterpret_cast<DefaultMQProducer*>(producer)->setRPCHook(rpcHook);
  return OK;
}

int SetProducerLogPath(CProducer* producer, const char* logPath) {
  if (producer == NULL) {
    return NULL_POINTER;
  }
  // TODO: This api should be implemented by core api.
  // reinterpret_cast<DefaultMQProducer*>(producer)->setLogFileSizeAndNum(3, 102400000);
  return OK;
}

int SetProducerLogFileNumAndSize(CProducer* producer, int fileNum, long fileSize) {
  if (producer == NULL) {
    return NULL_POINTER;
  }
  ALOG_ADAPTER->setLogFileNumAndSize(fileNum, fileSize);
  return OK;
}

int SetProducerLogLevel(CProducer* producer, CLogLevel level) {
  if (producer == NULL) {
    return NULL_POINTER;
  }
  ALOG_ADAPTER->setLogLevel((elogLevel)level);
  return OK;
}

int SetProducerSendMsgTimeout(CProducer* producer, int timeout) {
  if (producer == NULL) {
    return NULL_POINTER;
  }
  reinterpret_cast<DefaultMQProducer*>(producer)->setSendMsgTimeout(timeout);
  return OK;
}

int SetProducerCompressMsgBodyOverHowmuch(CProducer* producer, int howmuch) {
  if (producer == NULL) {
    return NULL_POINTER;
  }
  reinterpret_cast<DefaultMQProducer*>(producer)->setCompressMsgBodyOverHowmuch(howmuch);
  return OK;
}

int SetProducerCompressLevel(CProducer* producer, int level) {
  if (producer == NULL) {
    return NULL_POINTER;
  }
  reinterpret_cast<DefaultMQProducer*>(producer)->setCompressLevel(level);
  return OK;
}

int SetProducerMaxMessageSize(CProducer* producer, int size) {
  if (producer == NULL) {
    return NULL_POINTER;
  }
  reinterpret_cast<DefaultMQProducer*>(producer)->setMaxMessageSize(size);
  return OK;
}
