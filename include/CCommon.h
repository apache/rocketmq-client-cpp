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

#ifndef __C_COMMON_H__
#define __C_COMMON_H__

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_MESSAGE_ID_LENGTH 256
#define MAX_TOPIC_LENGTH 512
#define MAX_BROKER_NAME_ID_LENGTH 256
#define MAX_SDK_VERSION_LENGTH 256
#define DEFAULT_SDK_VERSION "DefaultVersion"
typedef enum _CStatus_ {
  // Success
  OK = 0,
  // Failed, null pointer value
  NULL_POINTER = 1,
  MALLOC_FAILED = 2,
  PRODUCER_ERROR_CODE_START = 10,
  PRODUCER_START_FAILED = 10,
  PRODUCER_SEND_SYNC_FAILED = 11,
  PRODUCER_SEND_ONEWAY_FAILED = 12,
  PRODUCER_SEND_ORDERLY_FAILED = 13,
  PRODUCER_SEND_ASYNC_FAILED = 14,
  PRODUCER_SEND_ORDERLYASYNC_FAILED = 15,
  PRODUCER_SEND_TRANSACTION_FAILED = 16,

  PUSHCONSUMER_ERROR_CODE_START = 20,
  PUSHCONSUMER_START_FAILED = 20,

  PULLCONSUMER_ERROR_CODE_START = 30,
  PULLCONSUMER_START_FAILED = 30,
  PULLCONSUMER_FETCH_MQ_FAILED = 31,
  PULLCONSUMER_FETCH_MESSAGE_FAILED = 32,

  Not_Support = 500,
  NOT_SUPPORT_NOW = -1
} CStatus;

typedef enum _CLogLevel_ {
  E_LOG_LEVEL_FATAL = 1,
  E_LOG_LEVEL_ERROR = 2,
  E_LOG_LEVEL_WARN = 3,
  E_LOG_LEVEL_INFO = 4,
  E_LOG_LEVEL_DEBUG = 5,
  E_LOG_LEVEL_TRACE = 6,
  E_LOG_LEVEL_LEVEL_NUM = 7
} CLogLevel;

#ifdef WIN32
#ifdef ROCKETMQCLIENT_EXPORTS
#ifdef _WINDLL
#define ROCKETMQCLIENT_API __declspec(dllexport)
#else
#define ROCKETMQCLIENT_API
#endif
#else
#ifdef ROCKETMQCLIENT_IMPORT
#define ROCKETMQCLIENT_API __declspec(dllimport)
#else
#define ROCKETMQCLIENT_API
#endif
#endif
#else
#define ROCKETMQCLIENT_API
#endif

typedef enum _CMessageModel_ { BROADCASTING, CLUSTERING } CMessageModel;
typedef enum _CTraceModel_ { OPEN, CLOSE } CTraceModel;

#ifdef __cplusplus
}
#endif
#endif  //__C_COMMON_H__
