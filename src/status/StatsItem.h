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

#ifndef __CONSUMER_STATUS_ITEM_H__
#define __CONSUMER_STATUS_ITEM_H__

#include "RocketMQClient.h"

namespace rocketmq {
class StatsItem {
 public:
  StatsItem() {
    pullRT = 0;
    pullRTCount = 0;
    pullCount = 0;
    consumeRT = 0;
    consumeRTCount = 0;
    consumeOKCount = 0;
    consumeFailedCount = 0;
    consumeFailedMsgs = 0;
  };
  virtual ~StatsItem() {}

 public:
  uint64 pullRT;
  uint64 pullRTCount;
  uint64 pullCount;
  uint64 consumeRT;
  uint64 consumeRTCount;
  uint64 consumeOKCount;
  uint64 consumeFailedCount;
  uint64 consumeFailedMsgs;
};

}  // namespace rocketmq

#endif
