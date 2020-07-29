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
#include "c/CMessage.h"

#include "MQMessage.h"

using namespace rocketmq;

CMessage* CreateMessage(const char* topic) {
  auto* msg = new MQMessage();
  if (topic != NULL) {
    msg->set_topic(topic);
  }
  return reinterpret_cast<CMessage*>(msg);
}

int DestroyMessage(CMessage* msg) {
  if (msg == NULL) {
    return NULL_POINTER;
  }
  delete reinterpret_cast<MQMessage*>(msg);
  return OK;
}

int SetMessageTopic(CMessage* msg, const char* topic) {
  if (msg == NULL) {
    return NULL_POINTER;
  }
  reinterpret_cast<MQMessage*>(msg)->set_topic(topic);
  return OK;
}

int SetMessageTags(CMessage* msg, const char* tags) {
  if (msg == NULL) {
    return NULL_POINTER;
  }
  reinterpret_cast<MQMessage*>(msg)->set_tags(tags);
  return OK;
}

int SetMessageKeys(CMessage* msg, const char* keys) {
  if (msg == NULL) {
    return NULL_POINTER;
  }
  reinterpret_cast<MQMessage*>(msg)->set_keys(keys);
  return OK;
}

int SetMessageBody(CMessage* msg, const char* body) {
  if (msg == NULL) {
    return NULL_POINTER;
  }
  reinterpret_cast<MQMessage*>(msg)->set_body(std::string(body));
  return OK;
}

int SetByteMessageBody(CMessage* msg, const char* body, int len) {
  if (msg == NULL) {
    return NULL_POINTER;
  }

  reinterpret_cast<MQMessage*>(msg)->set_body(std::string(body, len));
  return OK;
}

int SetMessageProperty(CMessage* msg, const char* key, const char* value) {
  if (msg == NULL) {
    return NULL_POINTER;
  }
  reinterpret_cast<MQMessage*>(msg)->putProperty(key, value);
  return OK;
}

int SetDelayTimeLevel(CMessage* msg, int level) {
  if (msg == NULL) {
    return NULL_POINTER;
  }
  reinterpret_cast<MQMessage*>(msg)->set_delay_time_level(level);
  return OK;
}

const char* GetOriginMessageTopic(CMessage* msg) {
  if (msg == NULL) {
    return NULL;
  }
  return reinterpret_cast<MQMessage*>(msg)->topic().c_str();
}

const char* GetOriginMessageTags(CMessage* msg) {
  if (msg == NULL) {
    return NULL;
  }
  return reinterpret_cast<MQMessage*>(msg)->tags().c_str();
}

const char* GetOriginMessageKeys(CMessage* msg) {
  if (msg == NULL) {
    return NULL;
  }
  return reinterpret_cast<MQMessage*>(msg)->keys().c_str();
}

const char* GetOriginMessageBody(CMessage* msg) {
  if (msg == NULL) {
    return NULL;
  }
  return reinterpret_cast<MQMessage*>(msg)->body().c_str();
}

const char* GetOriginMessageProperty(CMessage* msg, const char* key) {
  if (msg == NULL) {
    return NULL;
  }
  return reinterpret_cast<MQMessage*>(msg)->getProperty(key).c_str();
}

int GetOriginDelayTimeLevel(CMessage* msg) {
  if (msg == NULL) {
    return -1;
  }
  return reinterpret_cast<MQMessage*>(msg)->delay_time_level();
}
