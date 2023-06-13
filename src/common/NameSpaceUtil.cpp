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
#include "TraceConstant.h"

namespace rocketmq {

bool NameSpaceUtil::isEndPointURL(const string& nameServerAddr) {
  if (nameServerAddr.length() >= ENDPOINT_PREFIX_LENGTH && nameServerAddr.find(ENDPOINT_PREFIX) != string::npos) {
    return true;
  }
  return false;
}

string NameSpaceUtil::formatNameServerURL(const string& nameServerAddr) {
  auto index = nameServerAddr.find(ENDPOINT_PREFIX);
  if (index != string::npos) {
    LOG_DEBUG("Get Name Server from endpoint [%s]",
              nameServerAddr.substr(ENDPOINT_PREFIX_LENGTH, nameServerAddr.length() - ENDPOINT_PREFIX_LENGTH).c_str());
    return nameServerAddr.substr(ENDPOINT_PREFIX_LENGTH, nameServerAddr.length() - ENDPOINT_PREFIX_LENGTH);
  }
  return nameServerAddr;
}

string NameSpaceUtil::getNameSpaceFromNsURL(const string& nameServerAddr) {
  LOG_DEBUG("Try to get Name Space from nameServerAddr [%s]", nameServerAddr.c_str());
  string nsAddr = formatNameServerURL(nameServerAddr);
  string nameSpace;
  auto index = nsAddr.find(NAMESPACE_PREFIX);
  if (index != string::npos) {
    auto indexDot = nsAddr.find('.');
    if (indexDot != string::npos && indexDot > index) {
      nameSpace = nsAddr.substr(index, indexDot - index);
      LOG_INFO("Get Name Space [%s] from nameServerAddr [%s]", nameSpace.c_str(), nameServerAddr.c_str());
      return nameSpace;
    }
  }
  return "";
}

bool NameSpaceUtil::checkNameSpaceExistInNsURL(const string& nameServerAddr) {
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

bool NameSpaceUtil::checkNameSpaceExistInNameServer(const string& nameServerAddr) {
  auto index = nameServerAddr.find(NAMESPACE_PREFIX);
  if (index != string::npos) {
    LOG_INFO("Find Name Space Prefix in nameServerAddr [%s]", nameServerAddr.c_str());
    return true;
  }
  return false;
}

string NameSpaceUtil::withoutNameSpace(const string& source, const string& nameSpace) {
  if (!nameSpace.empty()) {
    auto index = source.find(nameSpace);
    if (index != string::npos) {
      return source.substr(index + nameSpace.length() + NAMESPACE_SPLIT_FLAG.length(), source.length());
    }
  }
  return source;
}
string NameSpaceUtil::withNameSpace(const string& source, const string& ns) {
  if (!ns.empty()) {
    return ns + NAMESPACE_SPLIT_FLAG + source;
  }
  return source;
}

bool NameSpaceUtil::hasNameSpace(const string& source, const string& ns) {
  if (source.find(TraceConstant::TRACE_TOPIC) != string::npos) {
    LOG_DEBUG("Find Trace Topic [%s]", source.c_str());
    return true;
  }
  if (!ns.empty() && source.length() >= ns.length() && source.find(ns) != string::npos) {
    return true;
  }
  return false;
}
}  // namespace rocketmq
