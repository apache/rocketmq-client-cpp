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

#ifndef __C_PULL_CONSUMER_H__
#define __C_PULL_CONSUMER_H__

#include "CCommon.h"
#include "CMessageExt.h"
#include "CMessageQueue.h"
#include "CPullResult.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef struct CPullConsumer CPullConsumer;


CPullConsumer *CreatePullConsumer(const char *groupId);
int DestroyPullConsumer(CPullConsumer *consumer);
int StartPullConsumer(CPullConsumer *consumer);
int ShutdownPullConsumer(CPullConsumer *consumer);
int SetPullConsumerGroupID(CPullConsumer *consumer, const char *groupId);
const char *GetPullConsumerGroupID(CPullConsumer *consumer);
int SetPullConsumerNameServerAddress(CPullConsumer *consumer, const char *namesrv);
int SetPullConsumerSessionCredentials(CPullConsumer *consumer, const char *accessKey, const char *secretKey,
                                     const char *channel);
int SetPullConsumerLogPath(CPullConsumer *consumer, const char *logPath);
int SetPullConsumerLogFileNumAndSize(CPullConsumer *consumer, int fileNum, long fileSize);
int SetPullConsumerLogLevel(CPullConsumer *consumer, CLogLevel level);

int FetchSubscriptionMessageQueues(CPullConsumer *consumer, const char *topic, CMessageQueue **mqs , int* size);
int ReleaseSubscriptionMessageQueue(CMessageQueue *mqs);

CPullResult Pull(CPullConsumer *consumer,const CMessageQueue *mq, const char *subExpression, long long offset, int maxNums);
int ReleasePullResult(CPullResult pullResult);
#ifdef __cplusplus
};
#endif
#endif //__C_PUSH_CONSUMER_H__
