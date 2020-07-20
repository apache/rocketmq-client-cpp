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
#include "LockBatchBody.h"

#include "Logging.h"
#include "MessageQueue.h"

namespace rocketmq {

std::string LockBatchRequestBody::getConsumerGroup() {
  return m_consumerGroup;
}

void LockBatchRequestBody::setConsumerGroup(std::string consumerGroup) {
  m_consumerGroup = consumerGroup;
}

std::string LockBatchRequestBody::getClientId() {
  return m_clientId;
}

void LockBatchRequestBody::setClientId(std::string clientId) {
  m_clientId = clientId;
}

std::vector<MQMessageQueue>& LockBatchRequestBody::getMqSet() {
  return m_mqSet;
}

void LockBatchRequestBody::setMqSet(std::vector<MQMessageQueue> mqSet) {
  m_mqSet.swap(mqSet);
}

std::string LockBatchRequestBody::encode() {
  Json::Value root;
  root["consumerGroup"] = m_consumerGroup;
  root["clientId"] = m_clientId;

  for (const auto& mq : m_mqSet) {
    root["mqSet"].append(rocketmq::toJson(mq));
  }

  return RemotingSerializable::toJson(root);
}

const std::vector<MQMessageQueue>& LockBatchResponseBody::getLockOKMQSet() {
  return m_lockOKMQSet;
}

void LockBatchResponseBody::setLockOKMQSet(std::vector<MQMessageQueue> lockOKMQSet) {
  m_lockOKMQSet.swap(lockOKMQSet);
}

LockBatchResponseBody* LockBatchResponseBody::Decode(const ByteArray& bodyData) {
  Json::Value root = RemotingSerializable::fromJson(bodyData);
  auto& mqs = root["lockOKMQSet"];
  std::unique_ptr<LockBatchResponseBody> body(new LockBatchResponseBody());
  for (const auto& qd : mqs) {
    MQMessageQueue mq(qd["topic"].asString(), qd["brokerName"].asString(), qd["queueId"].asInt());
    LOG_INFO("LockBatchResponseBody MQ:%s", mq.toString().c_str());
    body->m_lockOKMQSet.push_back(std::move(mq));
  }
  return body.release();
}

std::string UnlockBatchRequestBody::getConsumerGroup() {
  return m_consumerGroup;
}

void UnlockBatchRequestBody::setConsumerGroup(std::string consumerGroup) {
  m_consumerGroup = consumerGroup;
}

std::string UnlockBatchRequestBody::getClientId() {
  return m_clientId;
}

void UnlockBatchRequestBody::setClientId(std::string clientId) {
  m_clientId = clientId;
}

std::vector<MQMessageQueue>& UnlockBatchRequestBody::getMqSet() {
  return m_mqSet;
}

void UnlockBatchRequestBody::setMqSet(std::vector<MQMessageQueue> mqSet) {
  m_mqSet.swap(mqSet);
}

std::string UnlockBatchRequestBody::encode() {
  Json::Value root;
  root["consumerGroup"] = m_consumerGroup;
  root["clientId"] = m_clientId;

  for (const auto& mq : m_mqSet) {
    root["mqSet"].append(rocketmq::toJson(mq));
  }

  return RemotingSerializable::toJson(root);
}

}  // namespace rocketmq
