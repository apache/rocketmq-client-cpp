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
#include "MQClientImpl.h"

#include "Logging.h"
#include "MQAdminImpl.h"
#include "MQClientManager.h"
#include "TopicPublishInfo.hpp"
#include "UtilAll.h"

namespace rocketmq {

#define ROCKETMQCPP_VERSION "3.0.0"
#define BUILD_DATE __DATE__ " " __TIME__

// display version: strings bin/librocketmq.so |grep VERSION
const char* rocketmq_build_time = "VERSION: " ROCKETMQCPP_VERSION ", BUILD DATE: " BUILD_DATE;

void MQClientImpl::start() {
  if (nullptr == client_instance_) {
    if (nullptr == client_config_) {
      THROW_MQEXCEPTION(MQClientException, "have not clientConfig for create clientInstance.", -1);
    }

    client_instance_ = MQClientManager::getInstance()->getOrCreateMQClientInstance(*client_config_, rpc_hook_);
  }

  LOG_INFO_NEW("MQClientImpl start, clientId:{}, real nameservAddr:{}", client_instance_->getClientId(),
               client_instance_->getNamesrvAddr());
}

void MQClientImpl::shutdown() {
  client_instance_ = nullptr;
}

void MQClientImpl::createTopic(const std::string& key, const std::string& newTopic, int queueNum) {
  try {
    client_instance_->getMQAdminImpl()->createTopic(key, newTopic, queueNum);
  } catch (MQException& e) {
    LOG_ERROR(e.what());
  }
}

int64_t MQClientImpl::searchOffset(const MQMessageQueue& mq, int64_t timestamp) {
  return client_instance_->getMQAdminImpl()->searchOffset(mq, timestamp);
}

int64_t MQClientImpl::maxOffset(const MQMessageQueue& mq) {
  return client_instance_->getMQAdminImpl()->maxOffset(mq);
}

int64_t MQClientImpl::minOffset(const MQMessageQueue& mq) {
  return client_instance_->getMQAdminImpl()->minOffset(mq);
}

int64_t MQClientImpl::earliestMsgStoreTime(const MQMessageQueue& mq) {
  return client_instance_->getMQAdminImpl()->earliestMsgStoreTime(mq);
}

MQMessageExt MQClientImpl::viewMessage(const std::string& msgId) {
  return client_instance_->getMQAdminImpl()->viewMessage(msgId);
}

QueryResult MQClientImpl::queryMessage(const std::string& topic,
                                       const std::string& key,
                                       int maxNum,
                                       int64_t begin,
                                       int64_t end) {
  return client_instance_->getMQAdminImpl()->queryMessage(topic, key, maxNum, begin, end);
}

bool MQClientImpl::isServiceStateOk() {
  return service_state_ == RUNNING;
}

MQClientInstancePtr MQClientImpl::getClientInstance() const {
  return client_instance_;
}

void MQClientImpl::setClientInstance(MQClientInstancePtr clientInstance) {
  if (service_state_ == CREATE_JUST) {
    client_instance_ = clientInstance;
  } else {
    THROW_MQEXCEPTION(MQClientException, "Client already start, can not reset clientInstance!", -1);
  }
}

}  // namespace rocketmq
