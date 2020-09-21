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
#include "NamespaceUtil.h"

#include "Logging.h"
#include "UtilAll.h"

namespace rocketmq {

static const char NAMESPACE_SEPARATOR = '%';
static const std::string STRING_BLANK = "";
static const size_t RETRY_PREFIX_LENGTH = RETRY_GROUP_TOPIC_PREFIX.length();
static const size_t DLQ_PREFIX_LENGTH = DLQ_GROUP_TOPIC_PREFIX.length();

static const std::string ENDPOINT_PREFIX = "http://";
static const size_t ENDPOINT_PREFIX_LENGTH = ENDPOINT_PREFIX.length();

std::string NamespaceUtil::withoutNamespace(const std::string& resourceWithNamespace) {
  if (resourceWithNamespace.empty() || isSystemResource(resourceWithNamespace)) {
    return resourceWithNamespace;
  }

  auto resourceWithoutRetryAndDLQ = withoutRetryAndDLQ(resourceWithNamespace);
  auto index = resourceWithoutRetryAndDLQ.find(NAMESPACE_SEPARATOR);
  if (index > 0) {
    auto resourceWithoutNamespace = resourceWithoutRetryAndDLQ.substr(index + 1);
    if (UtilAll::isRetryTopic(resourceWithNamespace)) {
      return UtilAll::getRetryTopic(resourceWithoutNamespace);
    } else if (UtilAll::isDLQTopic(resourceWithNamespace)) {
      return UtilAll::getDLQTopic(resourceWithoutNamespace);
    } else {
      return resourceWithoutNamespace;
    }
  }

  return resourceWithNamespace;
}

std::string NamespaceUtil::withoutNamespace(const std::string& resourceWithNamespace, const std::string& name_space) {
  if (resourceWithNamespace.empty() || name_space.empty()) {
    return resourceWithNamespace;
  }

  auto resourceWithoutRetryAndDLQ = withoutRetryAndDLQ(resourceWithNamespace);
  if (resourceWithoutRetryAndDLQ.find(name_space + NAMESPACE_SEPARATOR) == 0) {
    return withoutNamespace(resourceWithNamespace);
  }

  return resourceWithNamespace;
}

std::string NamespaceUtil::wrapNamespace(const std::string& name_space, const std::string& resourceWithoutNamespace) {
  if (name_space.empty() || resourceWithoutNamespace.empty()) {
    return resourceWithoutNamespace;
  }

  // if (isSystemResource(resourceWithoutNamespace) || isAlreadyWithNamespace(resourceWithoutNamespace, namespace)) {
  //   return resourceWithoutNamespace;
  // }

  auto resourceWithoutRetryAndDLQ = withoutRetryAndDLQ(resourceWithoutNamespace);

  std::string resourceWithNamespace;

  if (UtilAll::isRetryTopic(resourceWithoutNamespace)) {
    resourceWithNamespace.append(RETRY_GROUP_TOPIC_PREFIX);
  }

  if (UtilAll::isDLQTopic(resourceWithoutNamespace)) {
    resourceWithNamespace.append(DLQ_GROUP_TOPIC_PREFIX);
  }

  resourceWithNamespace.append(name_space);
  resourceWithNamespace.push_back(NAMESPACE_SEPARATOR);
  resourceWithNamespace.append(resourceWithoutRetryAndDLQ);

  return resourceWithNamespace;
}

std::string NamespaceUtil::withoutRetryAndDLQ(const std::string& originalResource) {
  if (UtilAll::isRetryTopic(originalResource)) {
    return originalResource.substr(RETRY_PREFIX_LENGTH);
  } else if (UtilAll::isDLQTopic(originalResource)) {
    return originalResource.substr(DLQ_PREFIX_LENGTH);
  } else {
    return originalResource;
  }
}

bool NamespaceUtil::isSystemResource(const std::string& resource) {
  return false;
}

bool NamespaceUtil::isEndPointURL(const std::string& nameServerAddr) {
  return nameServerAddr.find(ENDPOINT_PREFIX) == 0;
}

std::string NamespaceUtil::formatNameServerURL(const std::string& nameServerAddr) {
  if (nameServerAddr.find(ENDPOINT_PREFIX) == 0) {
    return nameServerAddr.substr(ENDPOINT_PREFIX_LENGTH);
  }
  return nameServerAddr;
}

}  // namespace rocketmq
