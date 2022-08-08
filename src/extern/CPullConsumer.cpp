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

#include "CPullConsumer.h"
#include "CCommon.h"
#include "CMessageExt.h"
#include "DefaultMQPullConsumer.h"
#include "MQClientErrorContainer.h"
#include "Logging.h"

using namespace rocketmq;
using namespace std;

#ifdef __cplusplus
extern "C" {
#endif
char VERSION_FOR_PULL_CONSUMER[MAX_SDK_VERSION_LENGTH];
CPullConsumer* CreatePullConsumer(const char* groupId) {
  if (groupId == NULL) {
    return NULL;
  }
  DefaultMQPullConsumer* defaultMQPullConsumer = new DefaultMQPullConsumer(groupId);
  strncpy(VERSION_FOR_PULL_CONSUMER, defaultMQPullConsumer->version().c_str(), MAX_SDK_VERSION_LENGTH - 1);
  VERSION_FOR_PULL_CONSUMER[MAX_SDK_VERSION_LENGTH - 1] = 0;
  return (CPullConsumer*)defaultMQPullConsumer;
}
int DestroyPullConsumer(CPullConsumer* consumer) {
  if (consumer == NULL) {
    return NULL_POINTER;
  }
  delete reinterpret_cast<DefaultMQPullConsumer*>(consumer);
  return OK;
}
int StartPullConsumer(CPullConsumer* consumer) {
  if (consumer == NULL) {
    return NULL_POINTER;
  }
  try {
    ((DefaultMQPullConsumer*)consumer)->start();
  } catch (exception& e) {
    MQClientErrorContainer::setErr(string(e.what()));
    return PULLCONSUMER_START_FAILED;
  }
  return OK;
}
int ShutdownPullConsumer(CPullConsumer* consumer) {
  if (consumer == NULL) {
    return NULL_POINTER;
  }
  ((DefaultMQPullConsumer*)consumer)->shutdown();
  return OK;
}
const char* ShowPullConsumerVersion(CPullConsumer* consumer) {
  if (consumer == NULL) {
    return NULL;
  }
  return VERSION_FOR_PULL_CONSUMER;
}

int SetPullConsumerGroupID(CPullConsumer* consumer, const char* groupId) {
  if (consumer == NULL || groupId == NULL) {
    return NULL_POINTER;
  }
  ((DefaultMQPullConsumer*)consumer)->setGroupName(groupId);
  return OK;
}
const char* GetPullConsumerGroupID(CPullConsumer* consumer) {
  if (consumer == NULL) {
    return NULL;
  }
  return ((DefaultMQPullConsumer*)consumer)->getGroupName().c_str();
}
int SetPullConsumerNameServerAddress(CPullConsumer* consumer, const char* namesrv) {
  if (consumer == NULL) {
    return NULL_POINTER;
  }
  ((DefaultMQPullConsumer*)consumer)->setNamesrvAddr(namesrv);
  return OK;
}
int SetPullConsumerNameServerDomain(CPullConsumer* consumer, const char* domain) {
  if (consumer == NULL) {
    return NULL_POINTER;
  }
  ((DefaultMQPullConsumer*)consumer)->setNamesrvDomain(domain);
  return OK;
}
int SetPullConsumerSessionCredentials(CPullConsumer* consumer,
                                      const char* accessKey,
                                      const char* secretKey,
                                      const char* channel) {
  if (consumer == NULL) {
    return NULL_POINTER;
  }
  ((DefaultMQPullConsumer*)consumer)->setSessionCredentials(accessKey, secretKey, channel);
  return OK;
}

int SetPullConsumerLogPath(CPullConsumer* consumer, const char* logPath) {
  if (consumer == NULL) {
    return NULL_POINTER;
  }
  // Todo, This api should be implemented by core api.
  //((DefaultMQPullConsumer *) consumer)->setInstanceName(instanceName);
  setenv(ROCKETMQ_CLIENT_LOG_DIR.c_str(), logPath, 1);
  return OK;
}

int SetPullConsumerLogFileNumAndSize(CPullConsumer* consumer, int fileNum, long fileSize) {
  if (consumer == NULL) {
    return NULL_POINTER;
  }
  ((DefaultMQPullConsumer*)consumer)->setLogFileSizeAndNum(fileNum, fileSize);
  return OK;
}

int SetPullConsumerLogLevel(CPullConsumer* consumer, CLogLevel level) {
  if (consumer == NULL) {
    return NULL_POINTER;
  }
  ((DefaultMQPullConsumer*)consumer)->setLogLevel((elogLevel)level);
  return OK;
}

int FetchSubscriptionMessageQueues(CPullConsumer* consumer, const char* topic, CMessageQueue** mqs, int* size) {
  if (consumer == NULL) {
    return NULL_POINTER;
  }
  unsigned int index = 0;
  CMessageQueue* temMQ = NULL;
  std::vector<MQMessageQueue> fullMQ;
  try {
    ((DefaultMQPullConsumer*)consumer)->fetchSubscribeMessageQueues(topic, fullMQ);
    *size = fullMQ.size();
    // Alloc memory to save the pointer to CPP MessageQueue, and the MessageQueues may be changed.
    // Thus, this memory should be released by users using @ReleaseSubscribeMessageQueue every time.
    temMQ = (CMessageQueue*)malloc(*size * sizeof(CMessageQueue));
    if (temMQ == NULL) {
      *size = 0;
      *mqs = NULL;
      return MALLOC_FAILED;
    }
    auto iter = fullMQ.begin();
    for (index = 0; iter != fullMQ.end() && index <= fullMQ.size(); ++iter, index++) {
      strncpy(temMQ[index].topic, iter->getTopic().c_str(), MAX_TOPIC_LENGTH - 1);
      strncpy(temMQ[index].brokerName, iter->getBrokerName().c_str(), MAX_BROKER_NAME_ID_LENGTH - 1);
      temMQ[index].queueId = iter->getQueueId();
    }
    *mqs = temMQ;
  } catch (MQException& e) {
    *size = 0;
    *mqs = NULL;
    MQClientErrorContainer::setErr(string(e.what()));
    return PULLCONSUMER_FETCH_MQ_FAILED;
  }
  return OK;
}
int ReleaseSubscriptionMessageQueue(CMessageQueue* mqs) {
  if (mqs == NULL) {
    return NULL_POINTER;
  }
  free((void*)mqs);
  mqs = NULL;
  return OK;
}
CPullResult Pull(CPullConsumer* consumer,
                 const CMessageQueue* mq,
                 const char* subExpression,
                 long long offset,
                 int maxNums) {
  CPullResult pullResult;
  memset(&pullResult, 0, sizeof(CPullResult));
  if (consumer == NULL || subExpression == NULL) {
    pullResult.pullStatus = E_BROKER_TIMEOUT;
    return pullResult;
  }
  MQMessageQueue messageQueue(mq->topic, mq->brokerName, mq->queueId);
  PullResult cppPullResult;
  try {
    cppPullResult = ((DefaultMQPullConsumer*)consumer)->pull(messageQueue, subExpression, offset, maxNums);
  } catch (exception& e) {
    MQClientErrorContainer::setErr(string(e.what()));
    cppPullResult.pullStatus = BROKER_TIMEOUT;
  }

  if (cppPullResult.pullStatus != BROKER_TIMEOUT) {
    pullResult.maxOffset = cppPullResult.maxOffset;
    pullResult.minOffset = cppPullResult.minOffset;
    pullResult.nextBeginOffset = cppPullResult.nextBeginOffset;
  }

  switch (cppPullResult.pullStatus) {
    case FOUND: {
      pullResult.pullStatus = E_FOUND;
      pullResult.size = cppPullResult.msgFoundList.size();
      PullResult* tmpPullResult = new PullResult(cppPullResult);
      pullResult.pData = tmpPullResult;
      // Alloc memory to save the pointer to CPP MQMessageExt, which will be release by the CPP SDK core.
      // Thus, this memory should be released by users using @ReleasePullResult
      pullResult.msgFoundList = (CMessageExt**)malloc(pullResult.size * sizeof(CMessageExt*));
      for (size_t i = 0; i < cppPullResult.msgFoundList.size(); i++) {
        MQMessageExt* msg = const_cast<MQMessageExt*>(&tmpPullResult->msgFoundList[i]);
        pullResult.msgFoundList[i] = (CMessageExt*)(msg);
      }
      break;
    }
    case NO_NEW_MSG: {
      pullResult.pullStatus = E_NO_NEW_MSG;
      break;
    }
    case NO_MATCHED_MSG: {
      pullResult.pullStatus = E_NO_MATCHED_MSG;
      break;
    }
    case OFFSET_ILLEGAL: {
      pullResult.pullStatus = E_OFFSET_ILLEGAL;
      break;
    }
    case BROKER_TIMEOUT: {
      pullResult.pullStatus = E_BROKER_TIMEOUT;
      break;
    }
    default:
      pullResult.pullStatus = E_NO_NEW_MSG;
      break;
  }
  return pullResult;
}
int ReleasePullResult(CPullResult pullResult) {
  if (pullResult.size == 0 || pullResult.msgFoundList == NULL || pullResult.pData == NULL) {
    return NULL_POINTER;
  }
  if (pullResult.pData != NULL) {
    try {
      delete ((PullResult*)pullResult.pData);
    } catch (exception& e) {
      MQClientErrorContainer::setErr(string(e.what()));
      return NULL_POINTER;
    }
  }
  free((void*)pullResult.msgFoundList);
  pullResult.msgFoundList = NULL;
  return OK;
}

#ifdef __cplusplus
};
#endif
