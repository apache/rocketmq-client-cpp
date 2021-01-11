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
#ifndef ROCKETMQ_ROCKETMQCLIENT_H_
#define ROCKETMQ_ROCKETMQCLIENT_H_

#ifdef __cplusplus
#include <cstdint>
#include <cstddef>
#else
#include <stdint.h>
#include <stddef.h>
#endif

#ifdef WIN32
#ifdef ROCKETMQCLIENT_EXPORTS
#ifdef _WINDLL
#define ROCKETMQCLIENT_API __declspec(dllexport)
#else  // _WINDLL
#define ROCKETMQCLIENT_API
#endif  // _WINDLL
#else   // ROCKETMQCLIENT_EXPORTS
#ifdef ROCKETMQCLIENT_IMPORT
#define ROCKETMQCLIENT_API __declspec(dllimport)
#else  // ROCKETMQCLIENT_IMPORT
#define ROCKETMQCLIENT_API
#endif  // ROCKETMQCLIENT_IMPORT
#endif  // ROCKETMQCLIENT_EXPORTS
#else   // WIN32
#define ROCKETMQCLIENT_API
#endif  // WIN32

#ifdef WIN32
#define SIZET_FMT "%lu"
#else
#define SIZET_FMT "%zu"
#endif

#ifdef WIN32
#define FILE_SEPARATOR '\\'
#else
#define FILE_SEPARATOR '/'
#endif

#endif  // ROCKETMQ_ROCKETMQCLIENT_H_
