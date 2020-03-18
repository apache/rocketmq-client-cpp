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

#ifndef __C_PUSH_CONSUMER_H__
#define __C_PUSH_CONSUMER_H__

#include "CCommon.h"
#include "CMessageExt.h"

#ifdef __cplusplus
extern "C" {
#endif

// typedef struct _CConsumer_ _CConsumer;
typedef struct CPushConsumer CPushConsumer;

typedef enum E_CConsumeStatus { E_CONSUME_SUCCESS = 0, E_RECONSUME_LATER = 1 } CConsumeStatus;

typedef int (*MessageCallBack)(CPushConsumer*, CMessageExt*);

ROCKETMQCLIENT_API CPushConsumer* CreatePushConsumer(const char* groupId);
ROCKETMQCLIENT_API int DestroyPushConsumer(CPushConsumer* consumer);
ROCKETMQCLIENT_API int StartPushConsumer(CPushConsumer* consumer);
ROCKETMQCLIENT_API int ShutdownPushConsumer(CPushConsumer* consumer);
ROCKETMQCLIENT_API const char* ShowPushConsumerVersion(CPushConsumer* consumer);
ROCKETMQCLIENT_API int SetPushConsumerGroupID(CPushConsumer* consumer, const char* groupId);
ROCKETMQCLIENT_API const char* GetPushConsumerGroupID(CPushConsumer* consumer);
ROCKETMQCLIENT_API int SetPushConsumerNameServerAddress(CPushConsumer* consumer, const char* namesrv);
ROCKETMQCLIENT_API int SetPushConsumerNameServerDomain(CPushConsumer* consumer, const char* domain);
ROCKETMQCLIENT_API int Subscribe(CPushConsumer* consumer, const char* topic, const char* expression);
ROCKETMQCLIENT_API int RegisterMessageCallbackOrderly(CPushConsumer* consumer, MessageCallBack pCallback);
ROCKETMQCLIENT_API int RegisterMessageCallback(CPushConsumer* consumer, MessageCallBack pCallback);
ROCKETMQCLIENT_API int UnregisterMessageCallbackOrderly(CPushConsumer* consumer);
ROCKETMQCLIENT_API int UnregisterMessageCallback(CPushConsumer* consumer);
ROCKETMQCLIENT_API int SetPushConsumerThreadCount(CPushConsumer* consumer, int threadCount);
ROCKETMQCLIENT_API int SetPushConsumerMessageBatchMaxSize(CPushConsumer* consumer, int batchSize);
ROCKETMQCLIENT_API int SetPushConsumerInstanceName(CPushConsumer* consumer, const char* instanceName);
ROCKETMQCLIENT_API int SetPushConsumerSessionCredentials(CPushConsumer* consumer,
                                                         const char* accessKey,
                                                         const char* secretKey,
                                                         const char* channel);
ROCKETMQCLIENT_API int SetPushConsumerLogPath(CPushConsumer* consumer, const char* logPath);
ROCKETMQCLIENT_API int SetPushConsumerLogFileNumAndSize(CPushConsumer* consumer, int fileNum, long fileSize);
ROCKETMQCLIENT_API int SetPushConsumerLogLevel(CPushConsumer* consumer, CLogLevel level);
ROCKETMQCLIENT_API int SetPushConsumerMessageModel(CPushConsumer* consumer, CMessageModel messageModel);
ROCKETMQCLIENT_API int SetPushConsumerMaxCacheMessageSize(CPushConsumer* consumer, int maxCacheSize);
ROCKETMQCLIENT_API int SetPushConsumerMaxCacheMessageSizeInMb(CPushConsumer* consumer, int maxCacheSizeInMb);
ROCKETMQCLIENT_API int SetPushConsumerMessageTrace(CPushConsumer* consumer, CTraceModel openTrace);

#ifdef __cplusplus
}
#endif
#endif  //__C_PUSH_CONSUMER_H__
