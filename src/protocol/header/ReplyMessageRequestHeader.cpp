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
#include "ReplyMessageRequestHeader.h"

#include "UtilAll.h"

namespace rocketmq {

ReplyMessageRequestHeader* ReplyMessageRequestHeader::Decode(std::map<std::string, std::string>& extFields) {
  std::unique_ptr<ReplyMessageRequestHeader> header(new ReplyMessageRequestHeader());

  header->producerGroup = extFields.at("producerGroup");
  header->topic = extFields.at("topic");
  header->defaultTopic = extFields.at("defaultTopic");
  header->defaultTopicQueueNums = std::stoi(extFields.at("defaultTopicQueueNums"));
  header->queueId = std::stoi(extFields.at("queueId"));
  header->sysFlag = std::stoi(extFields.at("sysFlag"));
  header->bornTimestamp = std::stoll(extFields.at("bornTimestamp"));
  header->flag = std::stoi(extFields.at("flag"));

  auto it = extFields.find("properties");
  if (it != extFields.end()) {
    header->properties = it->second;
  }

  it = extFields.find("reconsumeTimes");
  if (it != extFields.end()) {
    header->reconsumeTimes = std::stoi(it->second);
  } else {
    header->reconsumeTimes = 0;
  }

  it = extFields.find("unitMode");
  if (it != extFields.end()) {
    header->unitMode = UtilAll::stob(it->second);
  } else {
    header->unitMode = false;
  }

  header->bornHost = extFields.at("bornHost");
  header->storeHost = extFields.at("storeHost");
  header->storeTimestamp = std::stoll(extFields.at("storeTimestamp"));

  return header.release();
}

const std::string& ReplyMessageRequestHeader::getProducerGroup() const {
  return this->producerGroup;
}

void ReplyMessageRequestHeader::setProducerGroup(const std::string& producerGroup) {
  this->producerGroup = producerGroup;
}

const std::string& ReplyMessageRequestHeader::getTopic() const {
  return this->topic;
}

void ReplyMessageRequestHeader::setTopic(const std::string& topic) {
  this->topic = topic;
}

const std::string& ReplyMessageRequestHeader::getDefaultTopic() const {
  return this->defaultTopic;
}

void ReplyMessageRequestHeader::setDefaultTopic(const std::string& defaultTopic) {
  this->defaultTopic = defaultTopic;
}

int32_t ReplyMessageRequestHeader::getDefaultTopicQueueNums() const {
  return this->defaultTopicQueueNums;
}

void ReplyMessageRequestHeader::setDefaultTopicQueueNums(int32_t defaultTopicQueueNums) {
  this->defaultTopicQueueNums = defaultTopicQueueNums;
}

int32_t ReplyMessageRequestHeader::getQueueId() const {
  return this->queueId;
}

void ReplyMessageRequestHeader::setQueueId(int32_t queueId) {
  this->queueId = queueId;
}

int32_t ReplyMessageRequestHeader::getSysFlag() const {
  return this->sysFlag;
}

void ReplyMessageRequestHeader::setSysFlag(int32_t sysFlag) {
  this->sysFlag = sysFlag;
}

int64_t ReplyMessageRequestHeader::getBornTimestamp() const {
  return this->bornTimestamp;
}

void ReplyMessageRequestHeader::setBornTimestamp(int64_t bornTimestamp) {
  this->bornTimestamp = bornTimestamp;
}

int32_t ReplyMessageRequestHeader::getFlag() const {
  return this->flag;
}

void ReplyMessageRequestHeader::setFlag(int32_t flag) {
  this->flag = flag;
}

const std::string& ReplyMessageRequestHeader::getProperties() const {
  return this->properties;
}

void ReplyMessageRequestHeader::setProperties(const std::string& properties) {
  this->properties = properties;
}

int32_t ReplyMessageRequestHeader::getReconsumeTimes() const {
  return this->reconsumeTimes;
}

void ReplyMessageRequestHeader::setReconsumeTimes(int32_t reconsumeTimes) {
  this->reconsumeTimes = reconsumeTimes;
}

bool ReplyMessageRequestHeader::getUnitMode() const {
  return this->unitMode;
}

void ReplyMessageRequestHeader::setUnitMode(bool unitMode) {
  this->unitMode = unitMode;
}

const std::string& ReplyMessageRequestHeader::getBornHost() const {
  return this->bornHost;
}

void ReplyMessageRequestHeader::setBornHost(const std::string& bornHost) {
  this->bornHost = bornHost;
}

const std::string& ReplyMessageRequestHeader::getStoreHost() const {
  return this->storeHost;
}

void ReplyMessageRequestHeader::setStoreHost(const std::string& storeHost) {
  this->storeHost = storeHost;
}

int64_t ReplyMessageRequestHeader::getStoreTimestamp() const {
  return this->storeTimestamp;
}

void ReplyMessageRequestHeader::setStoreTimestamp(int64_t storeTimestamp) {
  this->storeTimestamp = storeTimestamp;
}

}  // namespace rocketmq
