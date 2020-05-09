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
#include "DefaultMQProducerConfigImpl.h"

namespace rocketmq {

DefaultMQProducerConfigImpl::DefaultMQProducerConfigImpl()
    : m_maxMessageSize(1024 * 1024 * 4),       // 4MB
      m_compressMsgBodyOverHowmuch(1024 * 4),  // 4KB
      m_compressLevel(5),
      m_sendMsgTimeout(3000),
      m_retryTimes(2),
      m_retryTimes4Async(2),
      m_retryAnotherBrokerWhenNotStoreOK(false) {}

int DefaultMQProducerConfigImpl::getMaxMessageSize() const {
  return m_maxMessageSize;
}

void DefaultMQProducerConfigImpl::setMaxMessageSize(int maxMessageSize) {
  m_maxMessageSize = maxMessageSize;
}

int DefaultMQProducerConfigImpl::getCompressMsgBodyOverHowmuch() const {
  return m_compressMsgBodyOverHowmuch;
}

void DefaultMQProducerConfigImpl::setCompressMsgBodyOverHowmuch(int compressMsgBodyOverHowmuch) {
  m_compressMsgBodyOverHowmuch = compressMsgBodyOverHowmuch;
}

int DefaultMQProducerConfigImpl::getCompressLevel() const {
  return m_compressLevel;
}

void DefaultMQProducerConfigImpl::setCompressLevel(int compressLevel) {
  if ((compressLevel >= 0 && compressLevel <= 9) || compressLevel == -1) {
    m_compressLevel = compressLevel;
  }
}

int DefaultMQProducerConfigImpl::getSendMsgTimeout() const {
  return m_sendMsgTimeout;
}

void DefaultMQProducerConfigImpl::setSendMsgTimeout(int sendMsgTimeout) {
  m_sendMsgTimeout = sendMsgTimeout;
}

int DefaultMQProducerConfigImpl::getRetryTimes() const {
  return m_retryTimes;
}

void DefaultMQProducerConfigImpl::setRetryTimes(int times) {
  m_retryTimes = std::min(std::max(0, times), 15);
}

int DefaultMQProducerConfigImpl::getRetryTimes4Async() const {
  return m_retryTimes4Async;
}

void DefaultMQProducerConfigImpl::setRetryTimes4Async(int times) {
  m_retryTimes4Async = std::min(std::max(0, times), 15);
}

bool DefaultMQProducerConfigImpl::isRetryAnotherBrokerWhenNotStoreOK() const {
  return m_retryAnotherBrokerWhenNotStoreOK;
}

void DefaultMQProducerConfigImpl::setRetryAnotherBrokerWhenNotStoreOK(bool retryAnotherBrokerWhenNotStoreOK) {
  m_retryAnotherBrokerWhenNotStoreOK = retryAnotherBrokerWhenNotStoreOK;
}

}  // namespace rocketmq
