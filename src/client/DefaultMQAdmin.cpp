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

#include "DefaultMQAdmin.h"

namespace rocketmq {
DefaultMQAdmin::DefaultMQAdmin(const std::string& groupName) {
  //<!set default group name;
  string gname = groupName.empty() ? DEFAULT_ADMIN_GROUP : groupName;
  setGroupName(gname);
}

DefaultMQAdmin::~DefaultMQAdmin() {}

void DefaultMQAdmin::start() {
#ifndef WIN32
  /* Ignore the SIGPIPE */
  struct sigaction sa;
  memset(&sa, 0, sizeof(struct sigaction));
  sa.sa_handler = SIG_IGN;
  sa.sa_flags = 0;
  sigaction(SIGPIPE, &sa, 0);
#endif
  LOG_WARN("###Current Admin@%s", getClientVersionString().c_str());
  switch (m_serviceState) {
    case CREATE_JUST: {
      m_serviceState = START_FAILED;
      DefaultMQClient::start();
      LOG_INFO("DefaultMQAdmin:%s start", m_GroupName.c_str());
      bool registerOK = getFactory()->registerMQAdmin(this);
      if (!registerOK) {
        m_serviceState = CREATE_JUST;
        THROW_MQEXCEPTION(
            MQClientException,
            "The admin group[" + getGroupName() + "] has been created before, specify another name please.", -1);
      }
      getFactory()->start();
      m_serviceState = RUNNING;
      break;
    }
    case RUNNING:
    case START_FAILED:
    case SHUTDOWN_ALREADY:
      break;
    default:
      break;
  }
}

void DefaultMQAdmin::shutdown() {
  switch (m_serviceState) {
    case RUNNING: {
      LOG_INFO("DefaultMQAdmin:%s shutdown", m_GroupName.c_str());
      getFactory()->unregisterMQAdmin(this);
      getFactory()->shutdown();
      m_serviceState = SHUTDOWN_ALREADY;
      break;
    }
    case SHUTDOWN_ALREADY:
    case CREATE_JUST:
      break;
    default:
      break;
  }
}
}  // namespace rocketmq