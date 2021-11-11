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
#include "ClientConfigImpl.h"

#include <chrono>
#include <sstream>
#include <string>

#ifndef _WIN32
#include <unistd.h>
#endif

#include "UtilAll.h"

ROCKETMQ_NAMESPACE_BEGIN

#ifndef CLIENT_VERSION_MAJOR
#define CLIENT_VERSION_MAJOR "5"
#endif

#ifndef CLIENT_VERSION_MINOR
#define CLIENT_VERSION_MINOR "0"
#endif

#ifndef CLIENT_VERSION_PATCH
#define CLIENT_VERSION_PATCH "0"
#endif

const char* ClientConfigImpl::CLIENT_VERSION = CLIENT_VERSION_MAJOR "." CLIENT_VERSION_MINOR "." CLIENT_VERSION_PATCH;

std::string ClientConfigImpl::steadyName() {
  auto duration = std::chrono::steady_clock::now().time_since_epoch();
  return std::to_string(std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count());
}

ClientConfigImpl::ClientConfigImpl(absl::string_view group_name)
    : group_name_(group_name.data(), group_name.length()), io_timeout_(absl::Seconds(3)),
      long_polling_timeout_(absl::Seconds(30)) {
}

std::string ClientConfigImpl::clientId() const {
  std::stringstream ss;
  ss << UtilAll::hostname();
  ss << "@";
  std::string processID = std::to_string(getpid());
  ss << processID << "#";
  ss << instance_name_;
  return ss.str();
}

const std::string& ClientConfigImpl::getInstanceName() const {
  return instance_name_;
}

void ClientConfigImpl::setInstanceName(std::string instance_name) {
  instance_name_ = std::move(instance_name);
}

const std::string& ClientConfigImpl::getGroupName() const {
  return group_name_;
}

void ClientConfigImpl::setGroupName(std::string group_name) {
  group_name_ = std::move(group_name);
}

void ClientConfigImpl::setIoTimeout(absl::Duration timeout) {
  io_timeout_ = timeout;
}

void ClientConfigImpl::setCredentialsProvider(CredentialsProviderPtr credentials_provider) {
  credentials_provider_ = std::move(credentials_provider);
}

CredentialsProviderPtr ClientConfigImpl::credentialsProvider() {
  return credentials_provider_;
}

absl::Duration ClientConfigImpl::getIoTimeout() const {
  return io_timeout_;
}

bool operator==(const ClientConfigImpl& lhs, const ClientConfigImpl& rhs) {
  return lhs.resource_namespace_ == rhs.resource_namespace_ && lhs.transport_type_ == rhs.transport_type_;
}

ROCKETMQ_NAMESPACE_END