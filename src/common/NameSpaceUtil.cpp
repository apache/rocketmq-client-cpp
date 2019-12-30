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

string NameSpaceUtil::getNameSpaceFromNsURL(string nameServerAddr) {
  LOG_DEBUG("Try to get Name Space from nameServerAddr [%s]", nameServerAddr.c_str());
  string nsAddr = formatNameServerURL(nameServerAddr);
  string nameSpace;
  auto index = nameServerAddr.find(NAMESPACE_PREFIX);
  if (index != string::npos) {
    auto indexDot = nameServerAddr.find('.');
    if (indexDot != string::npos) {
      nameSpace = nameServerAddr.substr(index, indexDot);
      LOG_INFO("Get Name Space [%s] from nameServerAddr [%s]", nameSpace.c_str(), nameServerAddr.c_str());
      return nameSpace;
    }
  }
  return "";
}

bool NameSpaceUtil::checkNameSpaceExistInNsURL(string nameServerAddr) {
  if (!isEndPointURL(nameServerAddr)) {
    LOG_DEBUG("This nameServerAddr [%s] is not a endpoint. should not get Name Space.", nameServerAddr.c_str());
    return false;
  }
  auto index = nameServerAddr.find(NAMESPACE_PREFIX);
  if (index != string::npos) {
    LOG_INFO("Find Name Space Prefix in nameServerAddr [%s]", nameServerAddr.c_str());
    return true;
  }
  return false;
}

bool NameSpaceUtil::checkNameSpaceExistInNameServer(string nameServerAddr) {
  auto index = nameServerAddr.find(NAMESPACE_PREFIX);
  if (index != string::npos) {
    LOG_INFO("Find Name Space Prefix in nameServerAddr [%s]", nameServerAddr.c_str());
    return true;
  }
  return false;
}

string NameSpaceUtil::withNameSpace(string source, string ns) {
  if (!ns.empty()) {
    return ns + NAMESPACE_SPLIT_FLAG + source;
  }
  return source;
}

bool NameSpaceUtil::hasNameSpace(string source, string ns) {
  if (source.length() >= ns.length() && source.find(ns) != string::npos) {
    return true;
  }
  return false;
}
}  // namespace rocketmq
