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
#include "c/CMessageExt.h"

#include "MQMessageExt.h"

using namespace rocketmq;

const char* GetMessageTopic(CMessageExt* msg) {
  if (msg == NULL) {
    return NULL;
  }
  return reinterpret_cast<MQMessageExt*>(msg)->topic().c_str();
}

const char* GetMessageTags(CMessageExt* msg) {
  if (msg == NULL) {
    return NULL;
  }
  return reinterpret_cast<MQMessageExt*>(msg)->tags().c_str();
}

const char* GetMessageKeys(CMessageExt* msg) {
  if (msg == NULL) {
    return NULL;
  }
  return reinterpret_cast<MQMessageExt*>(msg)->keys().c_str();
}

const char* GetMessageBody(CMessageExt* msg) {
  if (msg == NULL) {
    return NULL;
  }
  return reinterpret_cast<MQMessageExt*>(msg)->body().c_str();
}

const char* GetMessageProperty(CMessageExt* msg, const char* key) {
  if (msg == NULL) {
    return NULL;
  }
  return reinterpret_cast<MQMessageExt*>(msg)->getProperty(key).c_str();
}

const char* GetMessageId(CMessageExt* msg) {
  if (msg == NULL) {
    return NULL;
  }
  return reinterpret_cast<MQMessageExt*>(msg)->msg_id().c_str();
}

int GetMessageDelayTimeLevel(CMessageExt* msg) {
  if (msg == NULL) {
    return NULL_POINTER;
  }
  return reinterpret_cast<MQMessageExt*>(msg)->delay_time_level();
}

int GetMessageQueueId(CMessageExt* msg) {
  if (msg == NULL) {
    return NULL_POINTER;
  }
  return reinterpret_cast<MQMessageExt*>(msg)->queue_id();
}

int GetMessageReconsumeTimes(CMessageExt* msg) {
  if (msg == NULL) {
    return NULL_POINTER;
  }
  return reinterpret_cast<MQMessageExt*>(msg)->reconsume_times();
}

int GetMessageStoreSize(CMessageExt* msg) {
  if (msg == NULL) {
    return NULL_POINTER;
  }
  return reinterpret_cast<MQMessageExt*>(msg)->store_size();
}

long long GetMessageBornTimestamp(CMessageExt* msg) {
  if (msg == NULL) {
    return NULL_POINTER;
  }
  return reinterpret_cast<MQMessageExt*>(msg)->born_timestamp();
}

long long GetMessageStoreTimestamp(CMessageExt* msg) {
  if (msg == NULL) {
    return NULL_POINTER;
  }
  return reinterpret_cast<MQMessageExt*>(msg)->store_timestamp();
}

long long GetMessageQueueOffset(CMessageExt* msg) {
  if (msg == NULL) {
    return NULL_POINTER;
  }
  return reinterpret_cast<MQMessageExt*>(msg)->queue_offset();
}

long long GetMessageCommitLogOffset(CMessageExt* msg) {
  if (msg == NULL) {
    return NULL_POINTER;
  }
  return reinterpret_cast<MQMessageExt*>(msg)->commit_log_offset();
}

long long GetMessagePreparedTransactionOffset(CMessageExt* msg) {
  if (msg == NULL) {
    return NULL_POINTER;
  }
  return reinterpret_cast<MQMessageExt*>(msg)->prepared_transaction_offset();
}
