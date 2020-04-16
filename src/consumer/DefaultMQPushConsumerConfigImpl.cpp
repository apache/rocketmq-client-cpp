
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
#include "DefaultMQPushConsumerConfigImpl.h"

#include <algorithm>
#include <thread>

#include "AllocateMQAveragely.h"

namespace rocketmq {

DefaultMQPushConsumerConfigImpl::DefaultMQPushConsumerConfigImpl()
    : m_messageModel(CLUSTERING),
      m_consumeFromWhere(CONSUME_FROM_LAST_OFFSET),
      m_consumeTimestamp("0"),
      m_consumeThreadNum(std::min(8, (int)std::thread::hardware_concurrency())),
      m_consumeMessageBatchMaxSize(1),
      m_maxMsgCacheSize(1000),
      m_asyncPullTimeout(30 * 1000),
      m_maxReconsumeTimes(-1),
      m_pullTimeDelayMillsWhenException(3000),
      m_allocateMQStrategy(new AllocateMQAveragely()) {}

MessageModel DefaultMQPushConsumerConfigImpl::getMessageModel() const {
  return m_messageModel;
}

void DefaultMQPushConsumerConfigImpl::setMessageModel(MessageModel messageModel) {
  m_messageModel = messageModel;
}

ConsumeFromWhere DefaultMQPushConsumerConfigImpl::getConsumeFromWhere() const {
  return m_consumeFromWhere;
}

void DefaultMQPushConsumerConfigImpl::setConsumeFromWhere(ConsumeFromWhere consumeFromWhere) {
  m_consumeFromWhere = consumeFromWhere;
}

std::string DefaultMQPushConsumerConfigImpl::getConsumeTimestamp() {
  return m_consumeTimestamp;
}

void DefaultMQPushConsumerConfigImpl::setConsumeTimestamp(std::string consumeTimestamp) {
  m_consumeTimestamp = consumeTimestamp;
}

int DefaultMQPushConsumerConfigImpl::getConsumeThreadNum() const {
  return m_consumeThreadNum;
}

void DefaultMQPushConsumerConfigImpl::setConsumeThreadNum(int threadNum) {
  if (threadNum > 0) {
    m_consumeThreadNum = threadNum;
  }
}

int DefaultMQPushConsumerConfigImpl::getConsumeMessageBatchMaxSize() const {
  return m_consumeMessageBatchMaxSize;
}

void DefaultMQPushConsumerConfigImpl::setConsumeMessageBatchMaxSize(int consumeMessageBatchMaxSize) {
  if (consumeMessageBatchMaxSize >= 1) {
    m_consumeMessageBatchMaxSize = consumeMessageBatchMaxSize;
  }
}

int DefaultMQPushConsumerConfigImpl::getMaxCacheMsgSizePerQueue() const {
  return m_maxMsgCacheSize;
}

void DefaultMQPushConsumerConfigImpl::setMaxCacheMsgSizePerQueue(int maxCacheSize) {
  if (maxCacheSize > 0 && maxCacheSize < 65535) {
    m_maxMsgCacheSize = maxCacheSize;
  }
}

int DefaultMQPushConsumerConfigImpl::getAsyncPullTimeout() const {
  return m_asyncPullTimeout;
}

void DefaultMQPushConsumerConfigImpl::setAsyncPullTimeout(int asyncPullTimeout) {
  m_asyncPullTimeout = asyncPullTimeout;
}

int DefaultMQPushConsumerConfigImpl::getMaxReconsumeTimes() const {
  return m_maxReconsumeTimes;
}

void DefaultMQPushConsumerConfigImpl::setMaxReconsumeTimes(int maxReconsumeTimes) {
  m_maxReconsumeTimes = maxReconsumeTimes;
}

long DefaultMQPushConsumerConfigImpl::getPullTimeDelayMillsWhenException() const {
  return m_pullTimeDelayMillsWhenException;
}

void DefaultMQPushConsumerConfigImpl::setPullTimeDelayMillsWhenException(long pullTimeDelayMillsWhenException) {
  m_pullTimeDelayMillsWhenException = pullTimeDelayMillsWhenException;
}

AllocateMQStrategy* DefaultMQPushConsumerConfigImpl::getAllocateMQStrategy() const {
  return m_allocateMQStrategy.get();
}

void DefaultMQPushConsumerConfigImpl::setAllocateMQStrategy(AllocateMQStrategy* strategy) {
  m_allocateMQStrategy.reset(strategy);
}

}  // namespace rocketmq
