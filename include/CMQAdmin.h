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

#ifndef __C_MQADMIN_H__
#define __C_MQADMIN_H__

#include "CCommon.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct CMQAdmin CMQAdmin;
ROCKETMQCLIENT_API CMQAdmin* CreateMQAdmin(const char* groupId);
ROCKETMQCLIENT_API int DestroyMQAdmin(CMQAdmin* MQAdmin);
ROCKETMQCLIENT_API int StartMQAdmin(CMQAdmin* MQAdmin);
ROCKETMQCLIENT_API int ShutdownMQAdmin(CMQAdmin* MQAdmin);
ROCKETMQCLIENT_API int SetSessionCredentialsMQAdmin(CMQAdmin* MQAdmin,
                                                    const char* accessKey,
                                                    const char* secretKey,
                                                    const char* channel);
ROCKETMQCLIENT_API int SetNamesrvAddrMQAdmin(CMQAdmin* MQAdmin, const char* addr);
ROCKETMQCLIENT_API int SetNamesrvDomainMQAdmin(CMQAdmin* MQAdmin, const char* domain);
ROCKETMQCLIENT_API int SetMQAdminLogPath(CMQAdmin* MQAdmin, const char* logPath);
ROCKETMQCLIENT_API int SetMQAdminLogFileNumAndSize(CMQAdmin* MQAdmin, int fileNum, long fileSize);
ROCKETMQCLIENT_API int SetMQAdminLogLevel(CMQAdmin* MQAdmin, CLogLevel level);
// you can add some api here
#ifdef __cplusplus
}
#endif
#endif  //__C_MQADMIN_H__
