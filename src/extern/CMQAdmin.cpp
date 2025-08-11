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
#include "CMQAdmin.h"
#include "DefaultMQAdmin.h"
#include "MQClientErrorContainer.h"

#ifdef __cplusplus
extern "C" {
#endif
using namespace rocketmq;
using namespace std;
CMQAdmin* CreateMQAdmin(const char* groupId) {
  if (groupId == NULL) {
    return NULL;
  }
  DefaultMQAdmin* defaultMQAdmin = new DefaultMQAdmin(groupId);
  return (CMQAdmin*)defaultMQAdmin;
}

int StartMQAdmin(CMQAdmin* MQAdmin) {
  if (MQAdmin == nullptr) {
    return NULL_POINTER;
  }
  DefaultMQAdmin* defaultMQAdmin = (DefaultMQAdmin*)MQAdmin;
  try {
    defaultMQAdmin->start();
  } catch (exception& e) {
    MQClientErrorContainer::setErr(string(e.what()));
    return ADMIN_START_FAILED;
  }
  return OK;
}

int DestroyMQAdmin(CMQAdmin* MQAdmin) {
  if (MQAdmin == nullptr) {
    return NULL_POINTER;
  }
  delete reinterpret_cast<DefaultMQAdmin*>(MQAdmin);
  return OK;
}

int ShutdownMQAdmin(CMQAdmin* MQAdmin) {
  if (MQAdmin == nullptr) {
    return NULL_POINTER;
  }
  DefaultMQAdmin* defaultMQAdmin = (DefaultMQAdmin*)MQAdmin;
  try {
    defaultMQAdmin->shutdown();
  } catch (exception& e) {
    MQClientErrorContainer::setErr(string(e.what()));
    return ADMIN_SHUTDOWN_FAILED;
  }
  return OK;
}

int SetSessionCredentialsMQAdmin(CMQAdmin* MQAdmin, const char* accessKey, const char* secretKey, const char* channel) {
  if (MQAdmin == nullptr) {
    return NULL_POINTER;
  }
  DefaultMQAdmin* defaultMQAdmin = (DefaultMQAdmin*)MQAdmin;
  try {
    defaultMQAdmin->setSessionCredentials(accessKey, secretKey, channel);
  } catch (exception& e) {
    MQClientErrorContainer::setErr(string(e.what()));
    return ADMIN_SET_VARIABLE_FAILED;
  }
  return OK;
}

int SetNamesrvAddrMQAdmin(CMQAdmin* MQAdmin, const char* addr) {
  if (MQAdmin == nullptr) {
    return NULL_POINTER;
  }
  DefaultMQAdmin* defaultMQAdmin = (DefaultMQAdmin*)MQAdmin;
  try {
    defaultMQAdmin->setNamesrvAddr(addr);
  } catch (exception& e) {
    MQClientErrorContainer::setErr(string(e.what()));
    return ADMIN_SET_VARIABLE_FAILED;
  }
  return OK;
}

int SetNamesrvDomainMQAdmin(CMQAdmin* MQAdmin, const char* domain) {
  if (MQAdmin == nullptr) {
    return NULL_POINTER;
  }
  DefaultMQAdmin* defaultMQAdmin = (DefaultMQAdmin*)MQAdmin;
  try {
    defaultMQAdmin->setNamesrvDomain(domain);
  } catch (exception& e) {
    MQClientErrorContainer::setErr(string(e.what()));
    return ADMIN_SET_VARIABLE_FAILED;
  }
  return OK;
}

int SetMQAdminLogPath(CMQAdmin* MQAdmin, const char* logPath) {
  if (MQAdmin == NULL) {
    return NULL_POINTER;
  }
  try {
    setenv(ROCKETMQ_CLIENT_LOG_DIR.c_str(), logPath, 1);
  } catch (exception& e) {
    MQClientErrorContainer::setErr(string(e.what()));
    return ADMIN_SET_VARIABLE_FAILED;
  }
  return OK;
}

int SetMQAdminLogFileNumAndSize(CMQAdmin* MQAdmin, int fileNum, long fileSize) {
  if (MQAdmin == NULL) {
    return NULL_POINTER;
  }
  if (fileNum <= 0 || fileSize <= 0) {
    return ADMIN_SET_VARIABLE_FAILED;
  }
  DefaultMQAdmin* defaultMQAdmin = (DefaultMQAdmin*)MQAdmin;
  try {
    defaultMQAdmin->setLogFileSizeAndNum(fileNum, fileSize);
  } catch (exception& e) {
    MQClientErrorContainer::setErr(string(e.what()));
    return ADMIN_SET_VARIABLE_FAILED;
  }
  return OK;
}

int SetMQAdminLogLevel(CMQAdmin* MQAdmin, CLogLevel level) {
  if (MQAdmin == NULL) {
    return NULL_POINTER;
  }
  DefaultMQAdmin* defaultMQAdmin = (DefaultMQAdmin*)MQAdmin;
  try {
    defaultMQAdmin->setLogLevel((elogLevel)level);
  } catch (exception& e) {
    MQClientErrorContainer::setErr(string(e.what()));
    return ADMIN_SET_VARIABLE_FAILED;
  }
  return OK;
}

#ifdef __cplusplus
};
#endif