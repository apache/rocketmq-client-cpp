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
#include "c/CPushConsumer.h"

#include <map>  // std::map

#include "ClientRPCHook.h"
#include "DefaultMQPushConsumer.h"
#include "Logging.h"
#include "MQClientErrorContainer.h"

using namespace rocketmq;

class MessageListenerInner : public MessageListenerConcurrently {
 public:
  MessageListenerInner(CPushConsumer* consumer, MessageCallBack callback)
      : m_consumer(consumer), m_msgReceivedCallback(callback) {}

  ~MessageListenerInner() = default;

  ConsumeStatus consumeMessage(const std::vector<MQMessageExt>& msgs) override {
    // to do user call back
    if (m_msgReceivedCallback == nullptr) {
      return RECONSUME_LATER;
    }
    for (auto msg : msgs) {
      auto* message = reinterpret_cast<CMessageExt*>(&msg);
      if (m_msgReceivedCallback(m_consumer, message) != E_CONSUME_SUCCESS) {
        return RECONSUME_LATER;
      }
    }
    return CONSUME_SUCCESS;
  }

 private:
  CPushConsumer* m_consumer;
  MessageCallBack m_msgReceivedCallback;
};

class MessageListenerOrderlyInner : public MessageListenerOrderly {
 public:
  MessageListenerOrderlyInner(CPushConsumer* consumer, MessageCallBack callback)
      : m_consumer(consumer), m_msgReceivedCallback(callback) {}

  ConsumeStatus consumeMessage(const std::vector<MQMessageExt>& msgs) override {
    if (m_msgReceivedCallback == nullptr) {
      return RECONSUME_LATER;
    }
    for (auto msg : msgs) {
      auto* message = reinterpret_cast<CMessageExt*>(&msg);
      if (m_msgReceivedCallback(m_consumer, message) != E_CONSUME_SUCCESS) {
        return RECONSUME_LATER;
      }
    }
    return CONSUME_SUCCESS;
  }

 private:
  CPushConsumer* m_consumer;
  MessageCallBack m_msgReceivedCallback;
};

CPushConsumer* CreatePushConsumer(const char* groupId) {
  if (groupId == NULL) {
    return NULL;
  }
  auto* defaultMQPushConsumer = new DefaultMQPushConsumer(groupId);
  defaultMQPushConsumer->setConsumeFromWhere(CONSUME_FROM_LAST_OFFSET);
  return reinterpret_cast<CPushConsumer*>(defaultMQPushConsumer);
}

int DestroyPushConsumer(CPushConsumer* consumer) {
  if (consumer == NULL) {
    return NULL_POINTER;
  }
  delete reinterpret_cast<DefaultMQPushConsumer*>(consumer);
  return OK;
}

int StartPushConsumer(CPushConsumer* consumer) {
  if (consumer == NULL) {
    return NULL_POINTER;
  }
  try {
    reinterpret_cast<DefaultMQPushConsumer*>(consumer)->start();
  } catch (std::exception& e) {
    MQClientErrorContainer::setErr(std::string(e.what()));
    return PUSHCONSUMER_START_FAILED;
  }
  return OK;
}

int ShutdownPushConsumer(CPushConsumer* consumer) {
  if (consumer == NULL) {
    return NULL_POINTER;
  }
  reinterpret_cast<DefaultMQPushConsumer*>(consumer)->shutdown();
  return OK;
}

int SetPushConsumerGroupID(CPushConsumer* consumer, const char* groupId) {
  if (consumer == NULL || groupId == NULL) {
    return NULL_POINTER;
  }
  reinterpret_cast<DefaultMQPushConsumer*>(consumer)->setGroupName(groupId);
  return OK;
}

const char* GetPushConsumerGroupID(CPushConsumer* consumer) {
  if (consumer == NULL) {
    return NULL;
  }
  return reinterpret_cast<DefaultMQPushConsumer*>(consumer)->getGroupName().c_str();
}

int SetPushConsumerNameServerAddress(CPushConsumer* consumer, const char* namesrv) {
  if (consumer == NULL) {
    return NULL_POINTER;
  }
  reinterpret_cast<DefaultMQPushConsumer*>(consumer)->setNamesrvAddr(namesrv);
  return OK;
}

// Deprecated
int SetPushConsumerNameServerDomain(CPushConsumer* consumer, const char* domain) {
  if (consumer == NULL) {
    return NULL_POINTER;
  }
  return NOT_SUPPORT_NOW;
}

int Subscribe(CPushConsumer* consumer, const char* topic, const char* expression) {
  if (consumer == NULL) {
    return NULL_POINTER;
  }
  reinterpret_cast<DefaultMQPushConsumer*>(consumer)->subscribe(topic, expression);
  return OK;
}

int RegisterMessageCallback(CPushConsumer* consumer, MessageCallBack callback) {
  if (consumer == NULL || callback == NULL) {
    return NULL_POINTER;
  }
  auto* listenerInner = new MessageListenerInner(consumer, callback);
  reinterpret_cast<DefaultMQPushConsumer*>(consumer)->registerMessageListener(listenerInner);
  return OK;
}

int RegisterMessageCallbackOrderly(CPushConsumer* consumer, MessageCallBack callback) {
  if (consumer == NULL || callback == NULL) {
    return NULL_POINTER;
  }
  auto* messageListenerOrderlyInner = new MessageListenerOrderlyInner(consumer, callback);
  reinterpret_cast<DefaultMQPushConsumer*>(consumer)->registerMessageListener(messageListenerOrderlyInner);
  return OK;
}

int UnregisterMessageCallbackOrderly(CPushConsumer* consumer) {
  if (consumer == NULL) {
    return NULL_POINTER;
  }
  auto* listenerInner = reinterpret_cast<DefaultMQPushConsumer*>(consumer)->getMessageListener();
  delete listenerInner;
  return OK;
}

int UnregisterMessageCallback(CPushConsumer* consumer) {
  if (consumer == NULL) {
    return NULL_POINTER;
  }
  auto* listenerInner = reinterpret_cast<DefaultMQPushConsumer*>(consumer)->getMessageListener();
  delete listenerInner;
  return OK;
}

int SetPushConsumerMessageModel(CPushConsumer* consumer, CMessageModel messageModel) {
  if (consumer == NULL) {
    return NULL_POINTER;
  }
  reinterpret_cast<DefaultMQPushConsumer*>(consumer)->setMessageModel(MessageModel((int)messageModel));
  return OK;
}

int SetPushConsumerThreadCount(CPushConsumer* consumer, int threadCount) {
  if (consumer == NULL || threadCount == 0) {
    return NULL_POINTER;
  }
  reinterpret_cast<DefaultMQPushConsumer*>(consumer)->setConsumeThreadNum(threadCount);
  return OK;
}

int SetPushConsumerMessageBatchMaxSize(CPushConsumer* consumer, int batchSize) {
  if (consumer == NULL || batchSize == 0) {
    return NULL_POINTER;
  }
  reinterpret_cast<DefaultMQPushConsumer*>(consumer)->setConsumeMessageBatchMaxSize(batchSize);
  return OK;
}

int SetPushConsumerMaxCacheMessageSize(CPushConsumer* consumer, int maxCacheSize) {
  if (consumer == NULL || maxCacheSize <= 0) {
    return NULL_POINTER;
  }
  reinterpret_cast<DefaultMQPushConsumer*>(consumer)->setMaxCacheMsgSizePerQueue(maxCacheSize);
  return OK;
}

int SetPushConsumerMaxCacheMessageSizeInMb(CPushConsumer* consumer, int maxCacheSizeInMb) {
  if (consumer == NULL || maxCacheSizeInMb <= 0) {
    return NULL_POINTER;
  }
  return NOT_SUPPORT_NOW;
}
int SetPushConsumerInstanceName(CPushConsumer* consumer, const char* instanceName) {
  if (consumer == NULL) {
    return NULL_POINTER;
  }
  reinterpret_cast<DefaultMQPushConsumer*>(consumer)->setInstanceName(instanceName);
  return OK;
}

int SetPushConsumerSessionCredentials(CPushConsumer* consumer,
                                      const char* accessKey,
                                      const char* secretKey,
                                      const char* channel) {
  if (consumer == NULL) {
    return NULL_POINTER;
  }
  auto rpcHook = std::make_shared<ClientRPCHook>(SessionCredentials(accessKey, secretKey, channel));
  reinterpret_cast<DefaultMQPushConsumer*>(consumer)->setRPCHook(rpcHook);
  return OK;
}

int SetPushConsumerLogPath(CPushConsumer* consumer, const char* logPath) {
  if (consumer == NULL) {
    return NULL_POINTER;
  }
  // TODO: This api should be implemented by core api.
  // reinterpret_cast<DefaultMQPushConsumer*>(consumer)->setInstanceName(instanceName);
  return OK;
}

int SetPushConsumerLogFileNumAndSize(CPushConsumer* consumer, int fileNum, long fileSize) {
  if (consumer == NULL) {
    return NULL_POINTER;
  }
  ALOG_ADAPTER->setLogFileNumAndSize(fileNum, fileSize);
  return OK;
}

int SetPushConsumerLogLevel(CPushConsumer* consumer, CLogLevel level) {
  if (consumer == NULL) {
    return NULL_POINTER;
  }
  ALOG_ADAPTER->setLogLevel((elogLevel)level);
  return OK;
}
