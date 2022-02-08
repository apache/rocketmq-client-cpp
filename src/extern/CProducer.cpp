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

#include "CProducer.h"
#include <string.h>
#include <functional>
#include <typeindex>
#include <typeinfo>
#include "AsyncCallback.h"
#include "CBatchMessage.h"
#include "CCommon.h"
#include "CMQException.h"
#include "CMessage.h"
#include "CSendResult.h"
#include "DefaultMQProducer.h"
#include "MQClient.h"
#include "MQClientErrorContainer.h"
#include "TransactionListener.h"
#include "TransactionMQProducer.h"
#include "TransactionSendResult.h"
#include "UtilAll.h"

#ifdef __cplusplus
extern "C" {
#endif
using namespace rocketmq;
using namespace std;
class MyLocalTransactionExecuterInner {
 public:
  MyLocalTransactionExecuterInner(CLocalTransactionExecutorCallback executor, CMessage* msg, void* userData)
      : m_ExcutorCallback(executor), message(msg), data(userData) {}
  ~MyLocalTransactionExecuterInner() {}
  CLocalTransactionExecutorCallback m_ExcutorCallback;
  CMessage* message;
  void* data;
};
class LocalTransactionListenerInner : public TransactionListener {
 public:
  LocalTransactionListenerInner() {}

  LocalTransactionListenerInner(CProducer* producer, CLocalTransactionCheckerCallback pCallback, void* data) {
    m_CheckerCallback = pCallback;
    m_producer = producer;
    m_data = data;
  }

  ~LocalTransactionListenerInner() {}

  LocalTransactionState executeLocalTransaction(const MQMessage& message, void* arg) {
    if (m_CheckerCallback == NULL) {
      return LocalTransactionState::UNKNOWN;
    }
    (void)(message);
    MyLocalTransactionExecuterInner* executerInner = (MyLocalTransactionExecuterInner*)arg;
    CTransactionStatus status =
        executerInner->m_ExcutorCallback(m_producer, executerInner->message, executerInner->data);
    switch (status) {
      case E_COMMIT_TRANSACTION:
        return LocalTransactionState::COMMIT_MESSAGE;

      case E_ROLLBACK_TRANSACTION:
        return LocalTransactionState::ROLLBACK_MESSAGE;

      default:
        return LocalTransactionState::UNKNOWN;
    }
  }

  LocalTransactionState checkLocalTransaction(const MQMessageExt& msg) {
    if (m_CheckerCallback == NULL) {
      return LocalTransactionState::UNKNOWN;
    }
    CMessageExt* msgExt = (CMessageExt*)(&msg);
    // CMessage *msg = (CMessage *) (&message);
    CTransactionStatus status = m_CheckerCallback(m_producer, msgExt, m_data);
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
  CLocalTransactionCheckerCallback m_CheckerCallback;
  CProducer* m_producer;
  void* m_data;
};

class SelectMessageQueueInner : public MessageQueueSelector {
 public:
  MQMessageQueue select(const std::vector<MQMessageQueue>& mqs, const MQMessage& msg, void* arg) {
    int index = 0;
    std::string shardingKey = rocketmq::UtilAll::to_string((char*)arg);

    index = std::hash<std::string>{}(shardingKey) % mqs.size();
    return mqs[index % mqs.size()];
  }
};

class SelectMessageQueue : public MessageQueueSelector {
 public:
  SelectMessageQueue(QueueSelectorCallback callback) { m_pCallback = callback; }

  MQMessageQueue select(const std::vector<MQMessageQueue>& mqs, const MQMessage& msg, void* arg) {
    CMessage* message = (CMessage*)&msg;
    // Get the index of sending MQMessageQueue through callback function.
    int index = m_pCallback(mqs.size(), message, arg);
    return mqs[index];
  }

 private:
  QueueSelectorCallback m_pCallback;
};
class COnSendCallback : public AutoDeleteSendCallBack {
 public:
  COnSendCallback(COnSendSuccessCallback cSendSuccessCallback,
                  COnSendExceptionCallback cSendExceptionCallback,
                  void* message,
                  void* userData) {
    m_cSendSuccessCallback = cSendSuccessCallback;
    m_cSendExceptionCallback = cSendExceptionCallback;
    m_message = message;
    m_userData = userData;
  }

  virtual ~COnSendCallback() {}

  virtual void onSuccess(SendResult& sendResult) {
    CSendResult result;
    result.sendStatus = CSendStatus((int)sendResult.getSendStatus());
    result.offset = sendResult.getQueueOffset();
    strncpy(result.msgId, sendResult.getMsgId().c_str(), MAX_MESSAGE_ID_LENGTH - 1);
    result.msgId[MAX_MESSAGE_ID_LENGTH - 1] = 0;
    m_cSendSuccessCallback(result, (CMessage*)m_message, m_userData);
  }

  virtual void onException(MQException& e) {
    CMQException exception;
    exception.error = e.GetError();
    exception.line = e.GetLine();
    strncpy(exception.msg, e.what(), MAX_EXEPTION_MSG_LENGTH - 1);
    strncpy(exception.file, e.GetFile(), MAX_EXEPTION_FILE_LENGTH - 1);
    m_cSendExceptionCallback(exception, (CMessage*)m_message, m_userData);
  }

 private:
  COnSendSuccessCallback m_cSendSuccessCallback;
  COnSendExceptionCallback m_cSendExceptionCallback;
  void* m_message;
  void* m_userData;
};

class CSendCallback : public AutoDeleteSendCallBack {
 public:
  CSendCallback(CSendSuccessCallback cSendSuccessCallback, CSendExceptionCallback cSendExceptionCallback) {
    m_cSendSuccessCallback = cSendSuccessCallback;
    m_cSendExceptionCallback = cSendExceptionCallback;
  }

  virtual ~CSendCallback() {}

  virtual void onSuccess(SendResult& sendResult) {
    CSendResult result;
    result.sendStatus = CSendStatus((int)sendResult.getSendStatus());
    result.offset = sendResult.getQueueOffset();
    strncpy(result.msgId, sendResult.getMsgId().c_str(), MAX_MESSAGE_ID_LENGTH - 1);
    result.msgId[MAX_MESSAGE_ID_LENGTH - 1] = 0;
    m_cSendSuccessCallback(result);
  }

  virtual void onException(MQException& e) {
    CMQException exception;
    exception.error = e.GetError();
    exception.line = e.GetLine();
    strncpy(exception.msg, e.what(), MAX_EXEPTION_MSG_LENGTH - 1);
    strncpy(exception.file, e.GetFile(), MAX_EXEPTION_FILE_LENGTH - 1);
    m_cSendExceptionCallback(exception);
  }

 private:
  CSendSuccessCallback m_cSendSuccessCallback;
  CSendExceptionCallback m_cSendExceptionCallback;
};
#ifndef CAPI_C_PRODUCER_TYPE_COMMON
#define CAPI_C_PRODUCER_TYPE_COMMON 0
#endif

#ifndef CAPI_C_PRODUCER_TYPE_ORDERLY
#define CAPI_C_PRODUCER_TYPE_ORDERLY 1
#endif

#ifndef CAPI_C_PRODUCER_TYPE_TRANSACTION
#define CAPI_C_PRODUCER_TYPE_TRANSACTION 2
#endif
typedef struct __DefaultProducer__ {
  DefaultMQProducer* innerProducer;
  TransactionMQProducer* innerTransactionProducer;
  LocalTransactionListenerInner* listenerInner;
  int producerType;
  char* version;
} DefaultProducer;
CProducer* CreateProducer(const char* groupId) {
  if (groupId == NULL) {
    return NULL;
  }
  DefaultProducer* defaultMQProducer = new DefaultProducer();
  defaultMQProducer->producerType = CAPI_C_PRODUCER_TYPE_COMMON;
  defaultMQProducer->innerProducer = new DefaultMQProducer(groupId);
  defaultMQProducer->version = new char[MAX_SDK_VERSION_LENGTH];
  strncpy(defaultMQProducer->version, defaultMQProducer->innerProducer->version().c_str(), MAX_SDK_VERSION_LENGTH - 1);
  defaultMQProducer->version[MAX_SDK_VERSION_LENGTH - 1] = 0;
  defaultMQProducer->innerTransactionProducer = NULL;
  defaultMQProducer->listenerInner = NULL;
  return (CProducer*)defaultMQProducer;
}

CProducer* CreateOrderlyProducer(const char* groupId) {
  if (groupId == NULL) {
    return NULL;
  }
  DefaultProducer* defaultMQProducer = new DefaultProducer();
  defaultMQProducer->producerType = CAPI_C_PRODUCER_TYPE_ORDERLY;
  defaultMQProducer->innerProducer = new DefaultMQProducer(groupId);

  defaultMQProducer->version = new char[MAX_SDK_VERSION_LENGTH];
  strncpy(defaultMQProducer->version, defaultMQProducer->innerProducer->version().c_str(), MAX_SDK_VERSION_LENGTH - 1);
  defaultMQProducer->version[MAX_SDK_VERSION_LENGTH - 1] = 0;
  defaultMQProducer->innerTransactionProducer = NULL;
  defaultMQProducer->listenerInner = NULL;
  return (CProducer*)defaultMQProducer;
}

CProducer* CreateTransactionProducer(const char* groupId, CLocalTransactionCheckerCallback callback, void* userData) {
  if (groupId == NULL) {
    return NULL;
  }
  DefaultProducer* defaultMQProducer = new DefaultProducer();
  defaultMQProducer->producerType = CAPI_C_PRODUCER_TYPE_TRANSACTION;
  defaultMQProducer->innerProducer = NULL;
  defaultMQProducer->innerTransactionProducer = new TransactionMQProducer(groupId);
  defaultMQProducer->listenerInner =
      new LocalTransactionListenerInner((CProducer*)defaultMQProducer, callback, userData);
  defaultMQProducer->innerTransactionProducer->setTransactionListener(defaultMQProducer->listenerInner);

  defaultMQProducer->version = new char[MAX_SDK_VERSION_LENGTH];
  strncpy(defaultMQProducer->version, defaultMQProducer->innerTransactionProducer->version().c_str(),
          MAX_SDK_VERSION_LENGTH - 1);
  defaultMQProducer->version[MAX_SDK_VERSION_LENGTH - 1] = 0;
  return (CProducer*)defaultMQProducer;
}
int DestroyProducer(CProducer* pProducer) {
  if (pProducer == NULL) {
    return NULL_POINTER;
  }
  DefaultProducer* defaultMQProducer = (DefaultProducer*)pProducer;
  if (defaultMQProducer->version != NULL) {
    delete[] defaultMQProducer->version;
    defaultMQProducer->version = NULL;
  }
  if (CAPI_C_PRODUCER_TYPE_TRANSACTION == defaultMQProducer->producerType) {
    if (defaultMQProducer->innerTransactionProducer != NULL) {
      delete defaultMQProducer->innerTransactionProducer;
      defaultMQProducer->innerTransactionProducer = NULL;
    }
  } else {
    if (defaultMQProducer->innerProducer != NULL) {
      delete defaultMQProducer->innerProducer;
      defaultMQProducer->innerProducer = NULL;
    }
  }
  delete reinterpret_cast<DefaultProducer*>(pProducer);
  return OK;
}
int StartProducer(CProducer* producer) {
  if (producer == NULL) {
    return NULL_POINTER;
  }
  DefaultProducer* defaultMQProducer = (DefaultProducer*)producer;
  try {
    if (CAPI_C_PRODUCER_TYPE_TRANSACTION == defaultMQProducer->producerType) {
      defaultMQProducer->innerTransactionProducer->start();
    } else {
      defaultMQProducer->innerProducer->start();
    }
  } catch (exception& e) {
    MQClientErrorContainer::setErr(string(e.what()));
    return PRODUCER_START_FAILED;
  }
  return OK;
}
int ShutdownProducer(CProducer* producer) {
  if (producer == NULL) {
    return NULL_POINTER;
  }
  DefaultProducer* defaultMQProducer = (DefaultProducer*)producer;
  try {
    if (CAPI_C_PRODUCER_TYPE_TRANSACTION == defaultMQProducer->producerType) {
      defaultMQProducer->innerTransactionProducer->shutdown();
    } else {
      defaultMQProducer->innerProducer->shutdown();
    }
  } catch (exception& e) {
    MQClientErrorContainer::setErr(string(e.what()));
    return PRODUCER_START_FAILED;
  }
  return OK;
}
const char* ShowProducerVersion(CProducer* producer) {
  if (producer == NULL) {
    return DEFAULT_SDK_VERSION;
  }
  DefaultProducer* defaultMQProducer = (DefaultProducer*)producer;

  return defaultMQProducer->version;
}
int SetProducerNameServerAddress(CProducer* producer, const char* namesrv) {
  if (producer == NULL) {
    return NULL_POINTER;
  }
  DefaultProducer* defaultMQProducer = (DefaultProducer*)producer;
  try {
    if (CAPI_C_PRODUCER_TYPE_TRANSACTION == defaultMQProducer->producerType) {
      defaultMQProducer->innerTransactionProducer->setNamesrvAddr(namesrv);
    } else {
      defaultMQProducer->innerProducer->setNamesrvAddr(namesrv);
    }
  } catch (exception& e) {
    MQClientErrorContainer::setErr(string(e.what()));
    return PRODUCER_START_FAILED;
  }
  return OK;
}
int SetProducerNameServerDomain(CProducer* producer, const char* domain) {
  if (producer == NULL) {
    return NULL_POINTER;
  }
  DefaultProducer* defaultMQProducer = (DefaultProducer*)producer;
  try {
    if (CAPI_C_PRODUCER_TYPE_TRANSACTION == defaultMQProducer->producerType) {
      defaultMQProducer->innerTransactionProducer->setNamesrvDomain(domain);
    } else {
      defaultMQProducer->innerProducer->setNamesrvDomain(domain);
    }
  } catch (exception& e) {
    MQClientErrorContainer::setErr(string(e.what()));
    return PRODUCER_START_FAILED;
  }
  return OK;
}
int SendMessageSync(CProducer* producer, CMessage* msg, CSendResult* result) {
  // CSendResult sendResult;
  if (producer == NULL || msg == NULL || result == NULL) {
    return NULL_POINTER;
  }
  try {
    DefaultProducer* defaultMQProducer = (DefaultProducer*)producer;
    MQMessage* message = (MQMessage*)msg;
    SendResult sendResult = defaultMQProducer->innerProducer->send(*message);
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
  } catch (exception& e) {
    MQClientErrorContainer::setErr(string(e.what()));
    return PRODUCER_SEND_SYNC_FAILED;
  }
  return OK;
}

int SendBatchMessage(CProducer* producer, CBatchMessage* batcMsg, CSendResult* result) {
  // CSendResult sendResult;
  if (producer == NULL || batcMsg == NULL || result == NULL) {
    return NULL_POINTER;
  }
  try {
    DefaultProducer* defaultMQProducer = (DefaultProducer*)producer;
    vector<MQMessage>* message = (vector<MQMessage>*)batcMsg;
    SendResult sendResult = defaultMQProducer->innerProducer->send(*message);
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
  } catch (exception& e) {
    return PRODUCER_SEND_SYNC_FAILED;
  }
  return OK;
}

int SendMessageAsync(CProducer* producer,
                     CMessage* msg,
                     CSendSuccessCallback cSendSuccessCallback,
                     CSendExceptionCallback cSendExceptionCallback) {
  if (producer == NULL || msg == NULL || cSendSuccessCallback == NULL || cSendExceptionCallback == NULL) {
    return NULL_POINTER;
  }
  DefaultProducer* defaultMQProducer = (DefaultProducer*)producer;
  MQMessage* message = (MQMessage*)msg;
  CSendCallback* cSendCallback = new CSendCallback(cSendSuccessCallback, cSendExceptionCallback);

  try {
    defaultMQProducer->innerProducer->send(*message, cSendCallback);
  } catch (exception& e) {
    if (cSendCallback != NULL) {
      if (std::type_index(typeid(e)) == std::type_index(typeid(MQException))) {
        MQException& mqe = (MQException&)e;
        cSendCallback->onException(mqe);
      }
      delete cSendCallback;
      cSendCallback = NULL;
    }
    MQClientErrorContainer::setErr(string(e.what()));
    return PRODUCER_SEND_ASYNC_FAILED;
  }
  return OK;
}

int SendAsync(CProducer* producer,
              CMessage* msg,
              COnSendSuccessCallback onSuccess,
              COnSendExceptionCallback onException,
              void* usrData) {
  if (producer == NULL || msg == NULL || onSuccess == NULL || onException == NULL) {
    return NULL_POINTER;
  }
  DefaultProducer* defaultMQProducer = (DefaultProducer*)producer;
  MQMessage* message = (MQMessage*)msg;
  COnSendCallback* cSendCallback = new COnSendCallback(onSuccess, onException, (void*)msg, usrData);

  try {
    defaultMQProducer->innerProducer->send(*message, cSendCallback);
  } catch (exception& e) {
    if (cSendCallback != NULL) {
      if (std::type_index(typeid(e)) == std::type_index(typeid(MQException))) {
        MQException& mqe = (MQException&)e;
        cSendCallback->onException(mqe);
      }
      delete cSendCallback;
      cSendCallback = NULL;
    }
    MQClientErrorContainer::setErr(string(e.what()));
    return PRODUCER_SEND_ASYNC_FAILED;
  }
  return OK;
}

int SendMessageOneway(CProducer* producer, CMessage* msg) {
  if (producer == NULL || msg == NULL) {
    return NULL_POINTER;
  }
  DefaultProducer* defaultMQProducer = (DefaultProducer*)producer;
  MQMessage* message = (MQMessage*)msg;
  try {
    defaultMQProducer->innerProducer->sendOneway(*message);
  } catch (exception& e) {
    return PRODUCER_SEND_ONEWAY_FAILED;
  }
  return OK;
}

int SendMessageOnewayOrderly(CProducer* producer, CMessage* msg, QueueSelectorCallback selector, void* arg) {
  if (producer == NULL || msg == NULL) {
    return NULL_POINTER;
  }
  DefaultProducer* defaultMQProducer = (DefaultProducer*)producer;
  MQMessage* message = (MQMessage*)msg;
  try {
    SelectMessageQueue selectMessageQueue(selector);
    defaultMQProducer->innerProducer->sendOneway(*message, &selectMessageQueue, arg);
  } catch (exception& e) {
    MQClientErrorContainer::setErr(string(e.what()));
    return PRODUCER_SEND_ONEWAY_FAILED;
  }
  return OK;
}

int SendMessageOrderlyAsync(CProducer* producer,
                            CMessage* msg,
                            QueueSelectorCallback callback,
                            void* arg,
                            CSendSuccessCallback cSendSuccessCallback,
                            CSendExceptionCallback cSendExceptionCallback) {
  if (producer == NULL || msg == NULL || callback == NULL || cSendSuccessCallback == NULL ||
      cSendExceptionCallback == NULL) {
    return NULL_POINTER;
  }
  DefaultProducer* defaultMQProducer = (DefaultProducer*)producer;
  MQMessage* message = (MQMessage*)msg;
  CSendCallback* cSendCallback = new CSendCallback(cSendSuccessCallback, cSendExceptionCallback);

  try {
    // Constructing SelectMessageQueue objects through function pointer callback
    SelectMessageQueue selectMessageQueue(callback);
    defaultMQProducer->innerProducer->send(*message, &selectMessageQueue, arg, cSendCallback);
  } catch (exception& e) {
    printf("%s\n", e.what());
    // std::count<<e.what( )<<std::endl;
    MQClientErrorContainer::setErr(string(e.what()));
    return PRODUCER_SEND_ORDERLYASYNC_FAILED;
  }
  return OK;
}

int SendMessageOrderly(CProducer* producer,
                       CMessage* msg,
                       QueueSelectorCallback callback,
                       void* arg,
                       int autoRetryTimes,
                       CSendResult* result) {
  if (producer == NULL || msg == NULL || callback == NULL || arg == NULL || result == NULL) {
    return NULL_POINTER;
  }
  DefaultProducer* defaultMQProducer = (DefaultProducer*)producer;
  MQMessage* message = (MQMessage*)msg;
  try {
    // Constructing SelectMessageQueue objects through function pointer callback
    SelectMessageQueue selectMessageQueue(callback);
    SendResult sendResult = defaultMQProducer->innerProducer->send(*message, &selectMessageQueue, arg, autoRetryTimes);
    // Convert SendStatus to CSendStatus
    result->sendStatus = CSendStatus((int)sendResult.getSendStatus());
    result->offset = sendResult.getQueueOffset();
    strncpy(result->msgId, sendResult.getMsgId().c_str(), MAX_MESSAGE_ID_LENGTH - 1);
    result->msgId[MAX_MESSAGE_ID_LENGTH - 1] = 0;
  } catch (exception& e) {
    MQClientErrorContainer::setErr(string(e.what()));
    return PRODUCER_SEND_ORDERLY_FAILED;
  }
  return OK;
}
int SendMessageOrderlyByShardingKey(CProducer* producer, CMessage* msg, const char* shardingKey, CSendResult* result) {
  if (producer == NULL || msg == NULL || shardingKey == NULL || result == NULL) {
    return NULL_POINTER;
  }
  DefaultProducer* defaultMQProducer = (DefaultProducer*)producer;
  MQMessage* message = (MQMessage*)msg;

  string sKey(shardingKey);
  message->setProperty("__SHARDINGKEY", sKey);
  try {
    // Constructing SelectMessageQueue objects through function pointer callback
    int retryTimes = 3;
    SelectMessageQueueInner selectMessageQueue;
    SendResult sendResult =
        defaultMQProducer->innerProducer->send(*message, &selectMessageQueue, (void*)shardingKey, retryTimes);
    // Convert SendStatus to CSendStatus
    result->sendStatus = CSendStatus((int)sendResult.getSendStatus());
    result->offset = sendResult.getQueueOffset();
    strncpy(result->msgId, sendResult.getMsgId().c_str(), MAX_MESSAGE_ID_LENGTH - 1);
    result->msgId[MAX_MESSAGE_ID_LENGTH - 1] = 0;
  } catch (exception& e) {
    MQClientErrorContainer::setErr(string(e.what()));
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
    DefaultProducer* defaultMQProducer = (DefaultProducer*)producer;
    MQMessage* message = (MQMessage*)msg;
    MyLocalTransactionExecuterInner executerInner(callback, msg, userData);
    // defaultMQProducer->listenerInner->setM_m_ExcutorCallback(callback);
    SendResult sendResult =
        defaultMQProducer->innerTransactionProducer->sendMessageInTransaction(*message, &executerInner);
    result->sendStatus = CSendStatus((int)sendResult.getSendStatus());
    result->offset = sendResult.getQueueOffset();
    strncpy(result->msgId, sendResult.getMsgId().c_str(), MAX_MESSAGE_ID_LENGTH - 1);
    result->msgId[MAX_MESSAGE_ID_LENGTH - 1] = 0;
  } catch (exception& e) {
    MQClientErrorContainer::setErr(string(e.what()));
    return PRODUCER_SEND_TRANSACTION_FAILED;
  }
  return OK;
}
int SetProducerGroupName(CProducer* producer, const char* groupName) {
  if (producer == NULL) {
    return NULL_POINTER;
  }
  DefaultProducer* defaultMQProducer = (DefaultProducer*)producer;
  try {
    if (CAPI_C_PRODUCER_TYPE_TRANSACTION == defaultMQProducer->producerType) {
      defaultMQProducer->innerTransactionProducer->setGroupName(groupName);
    } else {
      defaultMQProducer->innerProducer->setGroupName(groupName);
    }
  } catch (exception& e) {
    MQClientErrorContainer::setErr(string(e.what()));
    return PRODUCER_START_FAILED;
  }
  return OK;
}
int SetProducerInstanceName(CProducer* producer, const char* instanceName) {
  if (producer == NULL) {
    return NULL_POINTER;
  }
  DefaultProducer* defaultMQProducer = (DefaultProducer*)producer;
  try {
    if (CAPI_C_PRODUCER_TYPE_TRANSACTION == defaultMQProducer->producerType) {
      defaultMQProducer->innerTransactionProducer->setInstanceName(instanceName);
    } else {
      defaultMQProducer->innerProducer->setInstanceName(instanceName);
    }
  } catch (exception& e) {
    MQClientErrorContainer::setErr(string(e.what()));
    return PRODUCER_START_FAILED;
  }
  return OK;
}
int SetProducerSessionCredentials(CProducer* producer,
                                  const char* accessKey,
                                  const char* secretKey,
                                  const char* onsChannel) {
  if (producer == NULL) {
    return NULL_POINTER;
  }
  DefaultProducer* defaultMQProducer = (DefaultProducer*)producer;
  try {
    if (CAPI_C_PRODUCER_TYPE_TRANSACTION == defaultMQProducer->producerType) {
      defaultMQProducer->innerTransactionProducer->setSessionCredentials(accessKey, secretKey, onsChannel);
    } else {
      defaultMQProducer->innerProducer->setSessionCredentials(accessKey, secretKey, onsChannel);
    }
  } catch (exception& e) {
    MQClientErrorContainer::setErr(string(e.what()));
    return PRODUCER_START_FAILED;
  }
  return OK;
}
int SetProducerLogPath(CProducer* producer, const char* logPath) {
  if (producer == NULL) {
    return NULL_POINTER;
  }
  setenv(ROCKETMQ_CLIENT_LOG_DIR.c_str(), logPath, 1);
  return OK;
}

int SetProducerLogFileNumAndSize(CProducer* producer, int fileNum, long fileSize) {
  if (producer == NULL) {
    return NULL_POINTER;
  }
  DefaultProducer* defaultMQProducer = (DefaultProducer*)producer;
  try {
    if (CAPI_C_PRODUCER_TYPE_TRANSACTION == defaultMQProducer->producerType) {
      defaultMQProducer->innerTransactionProducer->setLogFileSizeAndNum(fileNum, fileSize);
    } else {
      defaultMQProducer->innerProducer->setLogFileSizeAndNum(fileNum, fileSize);
    }
  } catch (exception& e) {
    MQClientErrorContainer::setErr(string(e.what()));
    return PRODUCER_START_FAILED;
  }
  return OK;
}

int SetProducerLogLevel(CProducer* producer, CLogLevel level) {
  if (producer == NULL) {
    return NULL_POINTER;
  }
  DefaultProducer* defaultMQProducer = (DefaultProducer*)producer;
  try {
    if (CAPI_C_PRODUCER_TYPE_TRANSACTION == defaultMQProducer->producerType) {
      defaultMQProducer->innerTransactionProducer->setLogLevel((elogLevel)level);
    } else {
      defaultMQProducer->innerProducer->setLogLevel((elogLevel)level);
    }
  } catch (exception& e) {
    MQClientErrorContainer::setErr(string(e.what()));
    return PRODUCER_START_FAILED;
  }
  return OK;
}

int SetProducerSendMsgTimeout(CProducer* producer, int timeout) {
  if (producer == NULL) {
    return NULL_POINTER;
  }
  DefaultProducer* defaultMQProducer = (DefaultProducer*)producer;
  try {
    if (CAPI_C_PRODUCER_TYPE_TRANSACTION == defaultMQProducer->producerType) {
      defaultMQProducer->innerTransactionProducer->setSendMsgTimeout(timeout);
    } else {
      defaultMQProducer->innerProducer->setSendMsgTimeout(timeout);
    }
  } catch (exception& e) {
    MQClientErrorContainer::setErr(string(e.what()));
    return PRODUCER_START_FAILED;
  }
  return OK;
}

int SetProducerCompressMsgBodyOverHowmuch(CProducer* producer, int howmuch) {
  if (producer == NULL) {
    return NULL_POINTER;
  }
  DefaultProducer* defaultMQProducer = (DefaultProducer*)producer;
  try {
    if (CAPI_C_PRODUCER_TYPE_TRANSACTION == defaultMQProducer->producerType) {
      defaultMQProducer->innerTransactionProducer->setCompressMsgBodyOverHowmuch(howmuch);
    } else {
      defaultMQProducer->innerProducer->setCompressMsgBodyOverHowmuch(howmuch);
    }
  } catch (exception& e) {
    MQClientErrorContainer::setErr(string(e.what()));
    return PRODUCER_START_FAILED;
  }
  return OK;
}

int SetProducerCompressLevel(CProducer* producer, int level) {
  if (producer == NULL) {
    return NULL_POINTER;
  }
  DefaultProducer* defaultMQProducer = (DefaultProducer*)producer;
  try {
    if (CAPI_C_PRODUCER_TYPE_TRANSACTION == defaultMQProducer->producerType) {
      defaultMQProducer->innerTransactionProducer->setCompressLevel(level);
    } else {
      defaultMQProducer->innerProducer->setCompressLevel(level);
    }
  } catch (exception& e) {
    MQClientErrorContainer::setErr(string(e.what()));
    return PRODUCER_START_FAILED;
  }
  return OK;
}

int SetProducerMaxMessageSize(CProducer* producer, int size) {
  if (producer == NULL) {
    return NULL_POINTER;
  }
  DefaultProducer* defaultMQProducer = (DefaultProducer*)producer;
  try {
    if (CAPI_C_PRODUCER_TYPE_TRANSACTION == defaultMQProducer->producerType) {
      defaultMQProducer->innerTransactionProducer->setMaxMessageSize(size);
    } else {
      defaultMQProducer->innerProducer->setMaxMessageSize(size);
    }
  } catch (exception& e) {
    MQClientErrorContainer::setErr(string(e.what()));
    return PRODUCER_START_FAILED;
  }
  return OK;
}
int SetProducerMessageTrace(CProducer* producer, CTraceModel openTrace) {
  if (producer == NULL) {
    return NULL_POINTER;
  }
  DefaultProducer* defaultMQProducer = (DefaultProducer*)producer;
  bool messageTrace = openTrace == OPEN ? true : false;
  try {
    if (CAPI_C_PRODUCER_TYPE_TRANSACTION == defaultMQProducer->producerType) {
      defaultMQProducer->innerTransactionProducer->setMessageTrace(messageTrace);
    } else {
      defaultMQProducer->innerProducer->setMessageTrace(messageTrace);
    }
  } catch (exception& e) {
    MQClientErrorContainer::setErr(string(e.what()));
    return PRODUCER_START_FAILED;
  }
  return OK;
}
#ifdef __cplusplus
};
#endif
