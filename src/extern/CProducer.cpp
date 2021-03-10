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

#include "CErrorContainer.h"
#include "ClientRPCHook.h"
#include "DefaultMQProducer.h"
#include "Logging.h"
#include "TransactionMQProducer.h"
#include "UtilAll.h"

using namespace rocketmq;

class LocalTransactionExecutorInner {
 public:
  LocalTransactionExecutorInner(CLocalTransactionExecutorCallback callback, CMessage* message, void* userData)
      : excutor_callback_(callback), message_(message), user_data_(userData) {}

  ~LocalTransactionExecutorInner() = default;

 public:
  CLocalTransactionExecutorCallback excutor_callback_;
  CMessage* message_;
  void* user_data_;
};

class LocalTransactionListenerInner : public TransactionListener {
 public:
  LocalTransactionListenerInner(CProducer* producer, CLocalTransactionCheckerCallback callback, void* userData)
      : producer_(producer), checker_callback_(callback), user_data_(userData) {}

  ~LocalTransactionListenerInner() = default;

  LocalTransactionState executeLocalTransaction(const MQMessage& message, void* arg) override {
    if (checker_callback_ == nullptr) {
      return LocalTransactionState::UNKNOWN;
    }
    auto* msg = reinterpret_cast<CMessage*>(const_cast<MQMessage*>(&message));
    auto* executorInner = reinterpret_cast<LocalTransactionExecutorInner*>(arg);
    auto status = executorInner->excutor_callback_(producer_, msg, executorInner->user_data_);
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
    if (checker_callback_ == NULL) {
      return LocalTransactionState::UNKNOWN;
    }
    auto* msgExt = reinterpret_cast<CMessageExt*>(const_cast<MQMessageExt*>(&message));
    auto status = checker_callback_(producer_, msgExt, user_data_);
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
  CProducer* producer_;
  CLocalTransactionCheckerCallback checker_callback_;
  void* user_data_;
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
  SelectMessageQueue(QueueSelectorCallback callback) { callback_ = callback; }

  MQMessageQueue select(const std::vector<MQMessageQueue>& mqs, const MQMessage& msg, void* arg) override {
    auto* message = reinterpret_cast<CMessage*>(const_cast<MQMessage*>(&msg));
    // Get the index of sending MQMessageQueue through callback function.
    auto index = callback_(mqs.size(), message, arg);
    return mqs[index];
  }

 private:
  QueueSelectorCallback callback_;
};

class COnSendCallback : public AutoDeleteSendCallback {
 public:
  COnSendCallback(COnSendSuccessCallback sendSuccessCallback,
                  COnSendExceptionCallback sendExceptionCallback,
                  CMessage* message,
                  void* userData)
      : send_success_callback_(sendSuccessCallback),
        send_exception_callback_(sendExceptionCallback),
        message_(message),
        user_data_(userData) {}

  virtual ~COnSendCallback() = default;

  void onSuccess(SendResult& sendResult) override {
    CSendResult result;
    result.sendStatus = CSendStatus((int)sendResult.send_status());
    result.offset = sendResult.queue_offset();
    strncpy(result.msgId, sendResult.msg_id().c_str(), MAX_MESSAGE_ID_LENGTH - 1);
    result.msgId[MAX_MESSAGE_ID_LENGTH - 1] = 0;
    send_success_callback_(result, message_, user_data_);
  }

  void onException(MQException& e) noexcept override {
    CMQException exception;
    exception.error = e.GetError();
    exception.line = e.GetLine();
    strncpy(exception.msg, e.what(), MAX_EXEPTION_MSG_LENGTH - 1);
    strncpy(exception.file, e.GetFile(), MAX_EXEPTION_FILE_LENGTH - 1);
    send_exception_callback_(exception, message_, user_data_);
  }

 private:
  COnSendSuccessCallback send_success_callback_;
  COnSendExceptionCallback send_exception_callback_;
  CMessage* message_;
  void* user_data_;
};

class CSendCallback : public AutoDeleteSendCallback {
 public:
  CSendCallback(CSendSuccessCallback sendSuccessCallback, CSendExceptionCallback sendExceptionCallback)
      : send_success_callback_(sendSuccessCallback), send_exception_callback_(sendExceptionCallback) {}

  virtual ~CSendCallback() = default;

  void onSuccess(SendResult& sendResult) override {
    CSendResult result;
    result.sendStatus = CSendStatus((int)sendResult.send_status());
    result.offset = sendResult.queue_offset();
    strncpy(result.msgId, sendResult.msg_id().c_str(), MAX_MESSAGE_ID_LENGTH - 1);
    result.msgId[MAX_MESSAGE_ID_LENGTH - 1] = 0;
    send_success_callback_(result);
  }

  void onException(MQException& e) noexcept override {
    CMQException exception;
    exception.error = e.GetError();
    exception.line = e.GetLine();
    strncpy(exception.msg, e.what(), MAX_EXEPTION_MSG_LENGTH - 1);
    strncpy(exception.file, e.GetFile(), MAX_EXEPTION_FILE_LENGTH - 1);
    send_exception_callback_(exception);
  }

 private:
  CSendSuccessCallback send_success_callback_;
  CSendExceptionCallback send_exception_callback_;
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
    CErrorContainer::setErrorMessage(e.what());
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
  reinterpret_cast<DefaultMQProducer*>(producer)->set_namesrv_addr(namesrv);
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
    auto sendResult = defaultMQProducer->send(*message);
    switch (sendResult.send_status()) {
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
    result->offset = sendResult.queue_offset();
    strncpy(result->msgId, sendResult.msg_id().c_str(), MAX_MESSAGE_ID_LENGTH - 1);
    result->msgId[MAX_MESSAGE_ID_LENGTH - 1] = 0;
  } catch (std::exception& e) {
    CErrorContainer::setErrorMessage(e.what());
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
    auto* message = reinterpret_cast<std::vector<MQMessage>*>(batcMsg);
    auto sendResult = defaultMQProducer->send(*message);
    switch (sendResult.send_status()) {
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
    result->offset = sendResult.queue_offset();
    strncpy(result->msgId, sendResult.msg_id().c_str(), MAX_MESSAGE_ID_LENGTH - 1);
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
  defaultMQProducer->send(*message, sendCallback);
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
  defaultMQProducer->send(*message, sendCallback);
  return OK;
}

int SendMessageOneway(CProducer* producer, CMessage* msg) {
  if (producer == NULL || msg == NULL) {
    return NULL_POINTER;
  }
  auto* defaultMQProducer = reinterpret_cast<DefaultMQProducer*>(producer);
  auto* message = reinterpret_cast<MQMessage*>(msg);
  try {
    defaultMQProducer->sendOneway(*message);
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
    defaultMQProducer->sendOneway(*message, &selectMessageQueue, arg);
  } catch (std::exception& e) {
    CErrorContainer::setErrorMessage(e.what());
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
  defaultMQProducer->send(*message, &selectMessageQueue, arg, cSendCallback);
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
    SendResult send_result = defaultMQProducer->send(*message, &selectMessageQueue, arg);
    // Convert SendStatus to CSendStatus
    result->sendStatus = CSendStatus((int)send_result.send_status());
    result->offset = send_result.queue_offset();
    strncpy(result->msgId, send_result.msg_id().c_str(), MAX_MESSAGE_ID_LENGTH - 1);
    result->msgId[MAX_MESSAGE_ID_LENGTH - 1] = 0;
  } catch (std::exception& e) {
    CErrorContainer::setErrorMessage(e.what());
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
    SendResult send_esult = defaultMQProducer->send(*message, &selectMessageQueue, (void*)shardingKey, retryTimes);
    // Convert SendStatus to CSendStatus
    result->sendStatus = CSendStatus((int)send_esult.send_status());
    result->offset = send_esult.queue_offset();
    strncpy(result->msgId, send_esult.msg_id().c_str(), MAX_MESSAGE_ID_LENGTH - 1);
    result->msgId[MAX_MESSAGE_ID_LENGTH - 1] = 0;
  } catch (std::exception& e) {
    CErrorContainer::setErrorMessage(e.what());
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
    auto send_result = transactionMQProducer->sendMessageInTransaction(*message, &executorInner);
    result->sendStatus = CSendStatus((int)send_result.send_status());
    result->offset = send_result.queue_offset();
    strncpy(result->msgId, send_result.msg_id().c_str(), MAX_MESSAGE_ID_LENGTH - 1);
    result->msgId[MAX_MESSAGE_ID_LENGTH - 1] = 0;
  } catch (std::exception& e) {
    CErrorContainer::setErrorMessage(e.what());
    return PRODUCER_SEND_TRANSACTION_FAILED;
  }
  return OK;
}

int SetProducerGroupName(CProducer* producer, const char* groupName) {
  if (producer == NULL) {
    return NULL_POINTER;
  }
  reinterpret_cast<DefaultMQProducer*>(producer)->set_group_name(groupName);
  return OK;
}

int SetProducerInstanceName(CProducer* producer, const char* instanceName) {
  if (producer == NULL) {
    return NULL_POINTER;
  }
  reinterpret_cast<DefaultMQProducer*>(producer)->set_instance_name(instanceName);
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
  auto& default_logger_config = GetDefaultLoggerConfig();
  default_logger_config.set_file_count(fileNum);
  default_logger_config.set_file_size(fileSize);
  return OK;
}

int SetProducerLogLevel(CProducer* producer, CLogLevel level) {
  if (producer == NULL) {
    return NULL_POINTER;
  }
  auto& default_logger_config = GetDefaultLoggerConfig();
  default_logger_config.set_level(static_cast<LogLevel>(level));
  return OK;
}

int SetProducerSendMsgTimeout(CProducer* producer, int timeout) {
  if (producer == NULL) {
    return NULL_POINTER;
  }
  reinterpret_cast<DefaultMQProducer*>(producer)->set_send_msg_timeout(timeout);
  return OK;
}

int SetProducerCompressMsgBodyOverHowmuch(CProducer* producer, int howmuch) {
  if (producer == NULL) {
    return NULL_POINTER;
  }
  reinterpret_cast<DefaultMQProducer*>(producer)->set_compress_msg_body_over_howmuch(howmuch);
  return OK;
}

int SetProducerCompressLevel(CProducer* producer, int level) {
  if (producer == NULL) {
    return NULL_POINTER;
  }
  reinterpret_cast<DefaultMQProducer*>(producer)->set_compress_level(level);
  return OK;
}

int SetProducerMaxMessageSize(CProducer* producer, int size) {
  if (producer == NULL) {
    return NULL_POINTER;
  }
  reinterpret_cast<DefaultMQProducer*>(producer)->set_max_message_size(size);
  return OK;
}
