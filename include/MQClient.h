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

#ifndef __MQADMIN_H__
#define __MQADMIN_H__
#include "MQMessageExt.h"
#include "MQMessageQueue.h"
#include "RocketMQClient.h"
#include "SessionCredentials.h"

namespace rocketmq {

enum elogLevel {
  eLOG_LEVEL_FATAL = 1,
  eLOG_LEVEL_ERROR = 2,
  eLOG_LEVEL_WARN = 3,
  eLOG_LEVEL_INFO = 4,
  eLOG_LEVEL_DEBUG = 5,
  eLOG_LEVEL_TRACE = 6,
  eLOG_LEVEL_LEVEL_NUM = 7
};

}  // namespace rocketmq
#endif
