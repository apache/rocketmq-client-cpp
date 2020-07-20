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
#include "PullRequest.h"

#include <sstream>  // std::stringstream

#include "Logging.h"

namespace rocketmq {

PullRequest::PullRequest() : m_nextOffset(0), m_lockedFirst(false) {}

PullRequest::~PullRequest() {}

bool PullRequest::isLockedFirst() const {
  return m_lockedFirst;
}

void PullRequest::setLockedFirst(bool lockedFirst) {
  m_lockedFirst = lockedFirst;
}

const std::string& PullRequest::getConsumerGroup() const {
  return m_consumerGroup;
}

void PullRequest::setConsumerGroup(const std::string& consumerGroup) {
  m_consumerGroup = consumerGroup;
}

const MQMessageQueue& PullRequest::getMessageQueue() {
  return m_messageQueue;
}

void PullRequest::setMessageQueue(const MQMessageQueue& messageQueue) {
  m_messageQueue = messageQueue;
}

int64_t PullRequest::getNextOffset() {
  return m_nextOffset;
}

void PullRequest::setNextOffset(int64_t nextOffset) {
  m_nextOffset = nextOffset;
}

ProcessQueuePtr PullRequest::getProcessQueue() {
  return m_processQueue;
}

void PullRequest::setProcessQueue(ProcessQueuePtr processQueue) {
  m_processQueue = processQueue;
}

std::string PullRequest::toString() const {
  std::stringstream ss;
  ss << "PullRequest [consumerGroup=" << m_consumerGroup << ", messageQueue=" << m_messageQueue.toString()
     << ", nextOffset=" << m_nextOffset << "]";
  return ss.str();
}

}  // namespace rocketmq
