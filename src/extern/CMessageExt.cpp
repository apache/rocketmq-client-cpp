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

#include "MQMessageExt.h"
#include "CMessageExt.h"
#include "CCommon.h"

#ifdef __cplusplus
extern "C" {
#endif
using namespace rocketmq;
const char *GetMessageTopic(CMessageExt *msg) {
    if (msg == NULL) {
        return NULL;
    }
    return ((MQMessageExt *) msg)->getTopic().c_str();
}
const char *GetMessageTags(CMessageExt *msg) {
    if (msg == NULL) {
        return NULL;
    }
    return ((MQMessageExt *) msg)->getTags().c_str();
}
const char *GetMessageKeys(CMessageExt *msg) {
    if (msg == NULL) {
        return NULL;
    }
    return ((MQMessageExt *) msg)->getKeys().c_str();
}
const char *GetMessageBody(CMessageExt *msg) {
    if (msg == NULL) {
        return NULL;
    }
    return ((MQMessageExt *) msg)->getBody().c_str();
}
const char *GetMessageProperty(CMessageExt *msg, const char *key) {
    if (msg == NULL) {
        return NULL;
    }
    return ((MQMessageExt *) msg)->getProperty(key).c_str();
}
const char *GetMessageId(CMessageExt *msg) {
    if (msg == NULL) {
        return NULL;
    }
    return ((MQMessageExt *) msg)->getMsgId().c_str();
}

int GetMessageDelayTimeLevel(CMessageExt *msg){
    if (msg == NULL) {
        return NULL_POINTER;
    }
    return ((MQMessageExt *) msg)->getDelayTimeLevel();
}

int GetMessageQueueId(CMessageExt *msg){
    if (msg == NULL) {
        return NULL_POINTER;
    }
    return ((MQMessageExt *) msg)->getQueueId();
}

int GetMessageReconsumeTimes(CMessageExt *msg){
    if (msg == NULL) {
        return NULL_POINTER;
    }
    return ((MQMessageExt *) msg)->getReconsumeTimes();
}

int GetMessageStoreSize(CMessageExt *msg){
    if (msg == NULL) {
        return NULL_POINTER;
    }
    return ((MQMessageExt *) msg)->getStoreSize();
}

long long GetMessageBornTimestamp(CMessageExt *msg) {
    if (msg == NULL) {
        return NULL_POINTER;
    }
    return ((MQMessageExt *) msg)->getBornTimestamp();
}

long long GetMessageStoreTimestamp(CMessageExt *msg){
    if (msg == NULL) {
        return NULL_POINTER;
    }
    return ((MQMessageExt *) msg)->getBornTimestamp();
}

long long GetMessageQueueOffset(CMessageExt *msg){
    if (msg == NULL) {
        return NULL_POINTER;
    }
    return ((MQMessageExt *) msg)->getQueueOffset();
}

long long GetMessageCommitLogOffset(CMessageExt *msg){
    if (msg == NULL) {
        return NULL_POINTER;
    }
    return ((MQMessageExt *) msg)->getCommitLogOffset();
}

long long GetMessagePreparedTransactionOffset(CMessageExt *msg){
    if (msg == NULL) {
        return NULL_POINTER;
    }
    return ((MQMessageExt *) msg)->getPreparedTransactionOffset();
}
#ifdef __cplusplus
};
#endif
