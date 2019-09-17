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

#include "NameSpaceUtil.h"
#include "Logging.h"

namespace rocketmq {

bool NameSpaceUtil::isEndPointURL(string nameServerAddr) {
  if (nameServerAddr.length() >= ENDPOINT_PREFIX_LENGTH && nameServerAddr.find(ENDPOINT_PREFIX) != string::npos) {
    return true;
  }
  return false;
}

string NameSpaceUtil::formatNameServerURL(string nameServerAddr) {
  auto index = nameServerAddr.find(ENDPOINT_PREFIX);
  if (index != string::npos) {
    LOG_DEBUG("Get Name Server from endpoint [%s]",
              nameServerAddr.substr(ENDPOINT_PREFIX_LENGTH, nameServerAddr.length() - ENDPOINT_PREFIX_LENGTH).c_str());
    return nameServerAddr.substr(ENDPOINT_PREFIX_LENGTH, nameServerAddr.length() - ENDPOINT_PREFIX_LENGTH);
  }
  return nameServerAddr;
}
}  // namespace rocketmq
