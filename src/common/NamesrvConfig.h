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
#ifndef __NAMESRV_CONFIG_H__
#define __NAMESRV_CONFIG_H__

#include <string>

#include "UtilAll.h"

namespace rocketmq {

class NamesrvConfig {
 public:
  NamesrvConfig() {
    char* home = std::getenv(ROCKETMQ_HOME_ENV.c_str());
    if (home != nullptr) {
      m_rocketmqHome = home;
    }
  }

  const std::string& getRocketmqHome() const { return m_rocketmqHome; }

  void setRocketmqHome(const std::string& rocketmqHome) { m_rocketmqHome = rocketmqHome; }

  const std::string& getKvConfigPath() const { return m_kvConfigPath; }

  void setKvConfigPath(const std::string& kvConfigPath) { m_kvConfigPath = kvConfigPath; }

 private:
  std::string m_rocketmqHome;
  std::string m_kvConfigPath;
};

}  // namespace rocketmq

#endif  // __NAMESRV_CONFIG_H__
