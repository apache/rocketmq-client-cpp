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

#ifndef __C_PRODUCER_H__
#define __C_PRODUCER_H__

#include "CMessage.h"
#include "CSendResult.h"
#include "CMQException.h"

#ifdef __cplusplus
extern "C" {
#endif

//typedef struct _CProducer_ _CProducer;
typedef struct CProducer CProducer;
typedef int(*QueueSelectorCallback)(int size, CMessage *msg, void *arg);
typedef void(*CSendSuccessCallback)(CSendResult result);
typedef void(*CSendExceptionCallback)(CMQException e);

ROCKETMQCLIENT_API CProducer *CreateProducer(const char *groupId);
ROCKETMQCLIENT_API int DestroyProducer(CProducer *producer);
ROCKETMQCLIENT_API int StartProducer(CProducer *producer);
ROCKETMQCLIENT_API int ShutdownProducer(CProducer *producer);

ROCKETMQCLIENT_API int SetProducerNameServerAddress(CProducer *producer, const char *namesrv);
ROCKETMQCLIENT_API int SetProducerNameServerDomain(CProducer *producer, const char *domain);
ROCKETMQCLIENT_API int SetProducerGroupName(CProducer *producer, const char *groupName);
ROCKETMQCLIENT_API int SetProducerInstanceName(CProducer *producer, const char *instanceName);
ROCKETMQCLIENT_API int SetProducerSessionCredentials(CProducer *producer, const char *accessKey, const char *secretKey,
                                  const char *onsChannel);
ROCKETMQCLIENT_API int SetProducerLogPath(CProducer *producer, const char *logPath);
ROCKETMQCLIENT_API int SetProducerLogFileNumAndSize(CProducer *producer, int fileNum, long fileSize);
ROCKETMQCLIENT_API int SetProducerLogLevel(CProducer *producer, CLogLevel level);
ROCKETMQCLIENT_API int SetProducerSendMsgTimeout(CProducer *producer, int timeout);
ROCKETMQCLIENT_API int SetProducerCompressLevel(CProducer *producer, int level);
ROCKETMQCLIENT_API int SetProducerMaxMessageSize(CProducer *producer, int size);

ROCKETMQCLIENT_API int SendMessageSync(CProducer *producer, CMessage *msg, CSendResult *result);
ROCKETMQCLIENT_API int SendMessageAsync(CProducer *producer, CMessage *msg, CSendSuccessCallback cSendSuccessCallback , CSendExceptionCallback cSendExceptionCallback);
ROCKETMQCLIENT_API int SendMessageOneway(CProducer *producer,CMessage *msg);
ROCKETMQCLIENT_API int SendMessageOrderly(CProducer *producer, CMessage *msg, QueueSelectorCallback callback, void *arg, int autoRetryTimes, CSendResult *result);
#ifdef __cplusplus
};
#endif
#endif //__C_PRODUCER_H__
