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

#ifndef __C_MESSAGE_EXT_H__
#define __C_MESSAGE_EXT_H__

#include "CCommon.h"

#ifdef __cplusplus
extern "C" {
#endif

// typedef struct _CMessageExt_ _CMessageExt;
typedef struct CMessageExt CMessageExt;

ROCKETMQCLIENT_API const char* GetMessageTopic(CMessageExt* msgExt);
ROCKETMQCLIENT_API const char* GetMessageTags(CMessageExt* msgExt);
ROCKETMQCLIENT_API const char* GetMessageKeys(CMessageExt* msgExt);
ROCKETMQCLIENT_API const char* GetMessageBody(CMessageExt* msgExt);
ROCKETMQCLIENT_API const char* GetMessageProperty(CMessageExt* msgExt, const char* key);
ROCKETMQCLIENT_API const char* GetMessageId(CMessageExt* msgExt);
ROCKETMQCLIENT_API int GetMessageDelayTimeLevel(CMessageExt* msgExt);
ROCKETMQCLIENT_API int GetMessageQueueId(CMessageExt* msgExt);
ROCKETMQCLIENT_API int GetMessageReconsumeTimes(CMessageExt* msgExt);
ROCKETMQCLIENT_API int GetMessageStoreSize(CMessageExt* msgExt);
ROCKETMQCLIENT_API long long GetMessageBornTimestamp(CMessageExt* msgExt);
ROCKETMQCLIENT_API long long GetMessageStoreTimestamp(CMessageExt* msgExt);
ROCKETMQCLIENT_API long long GetMessageQueueOffset(CMessageExt* msgExt);
ROCKETMQCLIENT_API long long GetMessageCommitLogOffset(CMessageExt* msgExt);
ROCKETMQCLIENT_API long long GetMessagePreparedTransactionOffset(CMessageExt* msgExt);

#ifdef __cplusplus
}
#endif
#endif  //__C_MESSAGE_EXT_H__
