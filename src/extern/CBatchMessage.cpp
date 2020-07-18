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
#include "c/CBatchMessage.h"

#include <vector>

#include "MQMessage.h"

using namespace rocketmq;

CBatchMessage* CreateBatchMessage() {
  auto* msgs = new std::vector<MQMessage>();
  return reinterpret_cast<CBatchMessage*>(msgs);
}

int AddMessage(CBatchMessage* batchMsg, CMessage* msg) {
  if (msg == NULL) {
    return NULL_POINTER;
  }
  if (batchMsg == NULL) {
    return NULL_POINTER;
  }
  auto* message = reinterpret_cast<MQMessage*>(msg);
  reinterpret_cast<std::vector<MQMessage>*>(batchMsg)->push_back(*message);
  return OK;
}

int DestroyBatchMessage(CBatchMessage* batchMsg) {
  if (batchMsg == NULL) {
    return NULL_POINTER;
  }
  auto* msgs = reinterpret_cast<std::vector<MQMessage>*>(batchMsg);
  delete msgs;
  return OK;
}
