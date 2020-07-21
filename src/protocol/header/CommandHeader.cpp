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
#include "CommandHeader.h"

#include <sstream>

#include "Logging.h"
#include "MQException.h"
#include "RemotingSerializable.h"
#include "UtilAll.h"

namespace rocketmq {

//######################################
// GetRouteInfoRequestHeader
//######################################

void GetRouteInfoRequestHeader::Encode(Json::Value& extFields) {
  extFields["topic"] = topic;
}

void GetRouteInfoRequestHeader::SetDeclaredFieldOfCommandHeader(std::map<std::string, std::string>& requestMap) {
  requestMap.emplace("topic", topic);
}

//######################################
// UnregisterClientRequestHeader
//######################################

void UnregisterClientRequestHeader::Encode(Json::Value& extFields) {
  extFields["clientID"] = clientID;
  if (!producerGroup.empty()) {
    extFields["producerGroup"] = producerGroup;
  }
  if (!consumerGroup.empty()) {
    extFields["consumerGroup"] = consumerGroup;
  }
}

void UnregisterClientRequestHeader::SetDeclaredFieldOfCommandHeader(std::map<std::string, std::string>& requestMap) {
  requestMap.emplace("clientID", clientID);
  if (!producerGroup.empty()) {
    requestMap.emplace("producerGroup", producerGroup);
  }
  if (!consumerGroup.empty()) {
    requestMap.emplace("consumerGroup", consumerGroup);
  }
}

//######################################
// CreateTopicRequestHeader
//######################################

void CreateTopicRequestHeader::Encode(Json::Value& extFields) {
  extFields["topic"] = topic;
  extFields["defaultTopic"] = defaultTopic;
  extFields["readQueueNums"] = UtilAll::to_string(readQueueNums);
  extFields["writeQueueNums"] = UtilAll::to_string(writeQueueNums);
  extFields["perm"] = UtilAll::to_string(perm);
  extFields["topicFilterType"] = topicFilterType;
  if (topicSysFlag != -1) {
    extFields["topicSysFlag"] = UtilAll::to_string(topicSysFlag);
  }
  extFields["order"] = UtilAll::to_string(order);
}

void CreateTopicRequestHeader::SetDeclaredFieldOfCommandHeader(std::map<std::string, std::string>& requestMap) {
  requestMap.emplace("topic", topic);
  requestMap.emplace("defaultTopic", defaultTopic);
  requestMap.emplace("readQueueNums", UtilAll::to_string(readQueueNums));
  requestMap.emplace("writeQueueNums", UtilAll::to_string(writeQueueNums));
  requestMap.emplace("perm", UtilAll::to_string(perm));
  requestMap.emplace("topicFilterType", topicFilterType);
  if (topicSysFlag != -1) {
    requestMap.emplace("topicSysFlag", UtilAll::to_string(topicSysFlag));
  }
  requestMap.emplace("order", UtilAll::to_string(order));
}

//######################################
// CheckTransactionStateRequestHeader
//######################################

CheckTransactionStateRequestHeader* CheckTransactionStateRequestHeader::Decode(
    std::map<std::string, std::string>& extFields) {
  std::unique_ptr<CheckTransactionStateRequestHeader> header(new CheckTransactionStateRequestHeader());
  header->tranStateTableOffset = std::stoll(extFields.at("tranStateTableOffset"));
  header->commitLogOffset = std::stoll(extFields.at("commitLogOffset"));

  auto it = extFields.find("msgId");
  if (it != extFields.end()) {
    header->msgId = it->second;
  }

  it = extFields.find("transactionId");
  if (it != extFields.end()) {
    header->transactionId = it->second;
  }

  it = extFields.find("offsetMsgId");
  if (it != extFields.end()) {
    header->offsetMsgId = it->second;
  }

  return header.release();
}

void CheckTransactionStateRequestHeader::SetDeclaredFieldOfCommandHeader(
    std::map<std::string, std::string>& requestMap) {
  // TODO: unnecessary
  requestMap.emplace("tranStateTableOffset", UtilAll::to_string(tranStateTableOffset));
  requestMap.emplace("commitLogOffset", UtilAll::to_string(commitLogOffset));
  requestMap.emplace("msgId", msgId);
  requestMap.emplace("transactionId", transactionId);
  requestMap.emplace("offsetMsgId", offsetMsgId);
}

std::string CheckTransactionStateRequestHeader::toString() const {
  std::stringstream ss;
  ss << "CheckTransactionStateRequestHeader:";
  ss << " tranStateTableOffset:" << tranStateTableOffset;
  ss << " commitLogOffset:" << commitLogOffset;
  ss << " msgId:" << msgId;
  ss << " transactionId:" << transactionId;
  ss << " offsetMsgId:" << offsetMsgId;
  return ss.str();
}

//######################################
// EndTransactionRequestHeader
//######################################

void EndTransactionRequestHeader::Encode(Json::Value& extFields) {
  extFields["producerGroup"] = producerGroup;
  extFields["tranStateTableOffset"] = UtilAll::to_string(tranStateTableOffset);
  extFields["commitLogOffset"] = UtilAll::to_string(commitLogOffset);
  extFields["commitOrRollback"] = UtilAll::to_string(commitOrRollback);
  extFields["fromTransactionCheck"] = UtilAll::to_string(fromTransactionCheck);
  extFields["msgId"] = msgId;
  if (!transactionId.empty()) {
    extFields["transactionId"] = transactionId;
  }
}

void EndTransactionRequestHeader::SetDeclaredFieldOfCommandHeader(std::map<std::string, std::string>& requestMap) {
  requestMap.emplace("producerGroup", producerGroup);
  requestMap.emplace("tranStateTableOffset", UtilAll::to_string(tranStateTableOffset));
  requestMap.emplace("commitLogOffset", UtilAll::to_string(commitLogOffset));
  requestMap.emplace("commitOrRollback", UtilAll::to_string(commitOrRollback));
  requestMap.emplace("fromTransactionCheck", UtilAll::to_string(fromTransactionCheck));
  requestMap.emplace("msgId", msgId);
  if (!transactionId.empty()) {
    requestMap.emplace("transactionId", transactionId);
  }
}

std::string EndTransactionRequestHeader::toString() const {
  std::stringstream ss;
  ss << "EndTransactionRequestHeader:";
  ss << " m_producerGroup:" << producerGroup;
  ss << " m_tranStateTableOffset:" << tranStateTableOffset;
  ss << " m_commitLogOffset:" << commitLogOffset;
  ss << " m_commitOrRollback:" << commitOrRollback;
  ss << " m_fromTransactionCheck:" << fromTransactionCheck;
  ss << " m_msgId:" << msgId;
  ss << " m_transactionId:" << transactionId;
  return ss.str();
}

//######################################
// SendMessageRequestHeader
//######################################

void SendMessageRequestHeader::Encode(Json::Value& extFields) {
  extFields["producerGroup"] = producerGroup;
  extFields["topic"] = topic;
  extFields["defaultTopic"] = defaultTopic;
  extFields["defaultTopicQueueNums"] = UtilAll::to_string(defaultTopicQueueNums);
  extFields["queueId"] = UtilAll::to_string(queueId);
  extFields["sysFlag"] = UtilAll::to_string(sysFlag);
  extFields["bornTimestamp"] = UtilAll::to_string(bornTimestamp);
  extFields["flag"] = UtilAll::to_string(flag);
  if (!properties.empty()) {
    extFields["properties"] = properties;
  }
  if (reconsumeTimes != -1) {
    extFields["reconsumeTimes"] = UtilAll::to_string(reconsumeTimes);
  }
  extFields["unitMode"] = UtilAll::to_string(unitMode);
  extFields["batch"] = UtilAll::to_string(batch);
  if (maxReconsumeTimes != -1) {
    extFields["maxReconsumeTimes"] = UtilAll::to_string(maxReconsumeTimes);
  }
}

void SendMessageRequestHeader::SetDeclaredFieldOfCommandHeader(std::map<std::string, std::string>& requestMap) {
  LOG_DEBUG_NEW(
      "SendMessageRequestHeader producerGroup:{}, topic:%s, defaulttopic:{}, properties:{}, "
      "defaultTopicQueueNums:{}, queueId:{}, sysFlag:{}, bornTimestamp:{}, flag:{}",
      producerGroup, topic, defaultTopic, properties, defaultTopicQueueNums, queueId, sysFlag, bornTimestamp, flag);

  requestMap.emplace("producerGroup", producerGroup);
  requestMap.emplace("topic", topic);
  requestMap.emplace("defaultTopic", defaultTopic);
  requestMap.emplace("defaultTopicQueueNums", UtilAll::to_string(defaultTopicQueueNums));
  requestMap.emplace("queueId", UtilAll::to_string(queueId));
  requestMap.emplace("sysFlag", UtilAll::to_string(sysFlag));
  requestMap.emplace("bornTimestamp", UtilAll::to_string(bornTimestamp));
  requestMap.emplace("flag", UtilAll::to_string(flag));
  if (!properties.empty()) {
    requestMap.emplace("properties", properties);
  }
  if (reconsumeTimes != -1) {
    requestMap.emplace("reconsumeTimes", UtilAll::to_string(reconsumeTimes));
  }
  requestMap.emplace("unitMode", UtilAll::to_string(unitMode));
  requestMap.emplace("batch", UtilAll::to_string(batch));
  if (maxReconsumeTimes != -1) {
    requestMap.emplace("maxReconsumeTimes", UtilAll::to_string(maxReconsumeTimes));
  }
}

int SendMessageRequestHeader::getReconsumeTimes() {
  return reconsumeTimes;
}

void SendMessageRequestHeader::setReconsumeTimes(int _reconsumeTimes) {
  reconsumeTimes = _reconsumeTimes;
}

//######################################
// SendMessageRequestHeaderV2
//######################################

SendMessageRequestHeaderV2* SendMessageRequestHeaderV2::createSendMessageRequestHeaderV2(SendMessageRequestHeader* v1) {
  SendMessageRequestHeaderV2* v2 = new SendMessageRequestHeaderV2();
  v2->a = v1->producerGroup;
  v2->b = v1->topic;
  v2->c = v1->defaultTopic;
  v2->d = v1->defaultTopicQueueNums;
  v2->e = v1->queueId;
  v2->f = v1->sysFlag;
  v2->g = v1->bornTimestamp;
  v2->h = v1->flag;
  v2->i = v1->properties;
  v2->j = v1->reconsumeTimes;
  v2->k = v1->unitMode;
  v2->l = v1->maxReconsumeTimes;
  v2->m = v1->batch;
  return v2;
}

void SendMessageRequestHeaderV2::Encode(Json::Value& extFields) {
  extFields["a"] = a;
  extFields["b"] = b;
  extFields["c"] = c;
  extFields["d"] = UtilAll::to_string(d);
  extFields["e"] = UtilAll::to_string(e);
  extFields["f"] = UtilAll::to_string(f);
  extFields["g"] = UtilAll::to_string(g);
  extFields["h"] = UtilAll::to_string(h);
  if (!i.empty()) {
    extFields["i"] = i;
  }
  if (j != -1) {
    extFields["j"] = UtilAll::to_string(j);
  }
  extFields["k"] = UtilAll::to_string(k);
  if (l != -1) {
    extFields["l"] = UtilAll::to_string(l);
  }
  extFields["m"] = UtilAll::to_string(m);
}

void SendMessageRequestHeaderV2::SetDeclaredFieldOfCommandHeader(std::map<std::string, std::string>& requestMap) {
  requestMap.emplace("a", a);
  requestMap.emplace("b", b);
  requestMap.emplace("c", c);
  requestMap.emplace("d", UtilAll::to_string(d));
  requestMap.emplace("e", UtilAll::to_string(e));
  requestMap.emplace("f", UtilAll::to_string(f));
  requestMap.emplace("g", UtilAll::to_string(g));
  requestMap.emplace("h", UtilAll::to_string(h));
  if (!i.empty()) {
    requestMap.emplace("i", i);
  }
  if (j != -1) {
    requestMap.emplace("j", UtilAll::to_string(j));
  }
  requestMap.emplace("k", UtilAll::to_string(k));
  if (l != -1) {
    requestMap.emplace("l", UtilAll::to_string(l));
  }
  requestMap.emplace("m", UtilAll::to_string(m));
}

//######################################
// SendMessageResponseHeader
//######################################

SendMessageResponseHeader* SendMessageResponseHeader::Decode(std::map<std::string, std::string>& extFields) {
  std::unique_ptr<SendMessageResponseHeader> header(new SendMessageResponseHeader());
  header->msgId = extFields.at("msgId");
  header->queueId = std::stoi(extFields.at("queueId"));
  header->queueOffset = std::stoll(extFields.at("queueOffset"));

  const auto& it = extFields.find("transactionId");
  if (it != extFields.end()) {
    header->transactionId = it->second;
  }

  return header.release();
}

void SendMessageResponseHeader::SetDeclaredFieldOfCommandHeader(std::map<std::string, std::string>& requestMap) {
  // TODO: unnecessary
  requestMap.emplace("msgId", msgId);
  requestMap.emplace("queueId", UtilAll::to_string(queueId));
  requestMap.emplace("queueOffset", UtilAll::to_string(queueOffset));
  requestMap.emplace("transactionId", transactionId);
}

//######################################
// PullMessageRequestHeader
//######################################

void PullMessageRequestHeader::Encode(Json::Value& extFields) {
  extFields["consumerGroup"] = consumerGroup;
  extFields["topic"] = topic;
  extFields["queueId"] = queueId;
  extFields["queueOffset"] = UtilAll::to_string(queueOffset);
  extFields["maxMsgNums"] = maxMsgNums;
  extFields["sysFlag"] = sysFlag;
  extFields["commitOffset"] = UtilAll::to_string(commitOffset);
  extFields["suspendTimeoutMillis"] = UtilAll::to_string(suspendTimeoutMillis);
  if (!subscription.empty()) {
    extFields["subscription"] = subscription;
  }
  extFields["subVersion"] = UtilAll::to_string(subVersion);
  if (!expressionType.empty()) {
    extFields["expressionType"] = expressionType;
  }
}

void PullMessageRequestHeader::SetDeclaredFieldOfCommandHeader(std::map<std::string, std::string>& requestMap) {
  requestMap.emplace("consumerGroup", consumerGroup);
  requestMap.emplace("topic", topic);
  requestMap.emplace("queueId", UtilAll::to_string(queueId));
  requestMap.emplace("queueOffset", UtilAll::to_string(queueOffset));
  requestMap.emplace("maxMsgNums", UtilAll::to_string(maxMsgNums));
  requestMap.emplace("sysFlag", UtilAll::to_string(sysFlag));
  requestMap.emplace("commitOffset", UtilAll::to_string(commitOffset));
  requestMap.emplace("suspendTimeoutMillis", UtilAll::to_string(suspendTimeoutMillis));
  if (!subscription.empty()) {
    requestMap.emplace("subscription", subscription);
  }
  requestMap.emplace("subVersion", UtilAll::to_string(subVersion));
  if (!expressionType.empty()) {
    requestMap.emplace("expressionType", expressionType);
  }
}

//######################################
// PullMessageResponseHeader
//######################################

PullMessageResponseHeader* PullMessageResponseHeader::Decode(std::map<std::string, std::string>& extFields) {
  std::unique_ptr<PullMessageResponseHeader> header(new PullMessageResponseHeader());
  header->suggestWhichBrokerId = std::stoll(extFields.at("suggestWhichBrokerId"));
  header->nextBeginOffset = std::stoll(extFields.at("nextBeginOffset"));
  header->minOffset = std::stoll(extFields.at("minOffset"));
  header->maxOffset = std::stoll(extFields.at("maxOffset"));
  return header.release();
}

void PullMessageResponseHeader::SetDeclaredFieldOfCommandHeader(std::map<std::string, std::string>& requestMap) {
  // TODO: unnecessary
  requestMap.emplace("suggestWhichBrokerId", UtilAll::to_string(suggestWhichBrokerId));
  requestMap.emplace("nextBeginOffset", UtilAll::to_string(nextBeginOffset));
  requestMap.emplace("minOffset", UtilAll::to_string(minOffset));
  requestMap.emplace("maxOffset", UtilAll::to_string(maxOffset));
}

//######################################
// GetConsumerListByGroupResponseHeader
//######################################

void GetConsumerListByGroupResponseHeader::Encode(Json::Value& extFields) {}

void GetConsumerListByGroupResponseHeader::SetDeclaredFieldOfCommandHeader(
    std::map<std::string, std::string>& requestMap) {}

//######################################
// GetMinOffsetRequestHeader
//######################################

void GetMinOffsetRequestHeader::Encode(Json::Value& extFields) {
  extFields["topic"] = topic;
  extFields["queueId"] = UtilAll::to_string(queueId);
}

void GetMinOffsetRequestHeader::SetDeclaredFieldOfCommandHeader(std::map<std::string, std::string>& requestMap) {
  requestMap.emplace("topic", topic);
  requestMap.emplace("queueId", UtilAll::to_string(queueId));
}

//######################################
// GetMinOffsetResponseHeader
//######################################

GetMinOffsetResponseHeader* GetMinOffsetResponseHeader::Decode(std::map<std::string, std::string>& extFields) {
  std::unique_ptr<GetMinOffsetResponseHeader> header(new GetMinOffsetResponseHeader());
  header->offset = std::stoll(extFields.at("offset"));
  return header.release();
}

void GetMinOffsetResponseHeader::SetDeclaredFieldOfCommandHeader(std::map<std::string, std::string>& requestMap) {
  // TODO: unnecessary
  requestMap.emplace("offset", UtilAll::to_string(offset));
}

//######################################
// GetMaxOffsetRequestHeader
//######################################

void GetMaxOffsetRequestHeader::Encode(Json::Value& extFields) {
  extFields["topic"] = topic;
  extFields["queueId"] = UtilAll::to_string(queueId);
}

void GetMaxOffsetRequestHeader::SetDeclaredFieldOfCommandHeader(std::map<std::string, std::string>& requestMap) {
  requestMap.emplace("topic", topic);
  requestMap.emplace("queueId", UtilAll::to_string(queueId));
}

//######################################
// GetMaxOffsetResponseHeader
//######################################

GetMaxOffsetResponseHeader* GetMaxOffsetResponseHeader::Decode(std::map<std::string, std::string>& extFields) {
  std::unique_ptr<GetMaxOffsetResponseHeader> header(new GetMaxOffsetResponseHeader());
  header->offset = std::stoll(extFields.at("offset"));
  return header.release();
}

void GetMaxOffsetResponseHeader::SetDeclaredFieldOfCommandHeader(std::map<std::string, std::string>& requestMap) {
  // TODO: unnecessary
  requestMap.emplace("offset", UtilAll::to_string(offset));
}

//######################################
// SearchOffsetRequestHeader
//######################################

void SearchOffsetRequestHeader::Encode(Json::Value& extFields) {
  extFields["topic"] = topic;
  extFields["queueId"] = UtilAll::to_string(queueId);
  extFields["timestamp"] = UtilAll::to_string(timestamp);
}

void SearchOffsetRequestHeader::SetDeclaredFieldOfCommandHeader(std::map<std::string, std::string>& requestMap) {
  requestMap.emplace("topic", topic);
  requestMap.emplace("queueId", UtilAll::to_string(queueId));
  requestMap.emplace("timestamp", UtilAll::to_string(timestamp));
}

//######################################
// SearchOffsetResponseHeader
//######################################

SearchOffsetResponseHeader* SearchOffsetResponseHeader::Decode(std::map<std::string, std::string>& extFields) {
  std::unique_ptr<SearchOffsetResponseHeader> header(new SearchOffsetResponseHeader());
  header->offset = std::stoll(extFields.at("offset"));
  return header.release();
}

void SearchOffsetResponseHeader::SetDeclaredFieldOfCommandHeader(std::map<std::string, std::string>& requestMap) {
  // TODO: unnecessary
  requestMap.emplace("offset", UtilAll::to_string(offset));
}

//######################################
// ViewMessageRequestHeader
//######################################

void ViewMessageRequestHeader::Encode(Json::Value& extFields) {
  extFields["offset"] = UtilAll::to_string(offset);
}

void ViewMessageRequestHeader::SetDeclaredFieldOfCommandHeader(std::map<std::string, std::string>& requestMap) {
  requestMap.emplace("offset", UtilAll::to_string(offset));
}

//######################################
// GetEarliestMsgStoretimeRequestHeader
//######################################

void GetEarliestMsgStoretimeRequestHeader::Encode(Json::Value& extFields) {
  extFields["topic"] = topic;
  extFields["queueId"] = UtilAll::to_string(queueId);
}

void GetEarliestMsgStoretimeRequestHeader::SetDeclaredFieldOfCommandHeader(
    std::map<std::string, std::string>& requestMap) {
  requestMap.emplace("topic", topic);
  requestMap.emplace("queueId", UtilAll::to_string(queueId));
}

//######################################
// GetEarliestMsgStoretimeResponseHeader
//######################################

GetEarliestMsgStoretimeResponseHeader* GetEarliestMsgStoretimeResponseHeader::Decode(
    std::map<std::string, std::string>& extFields) {
  std::unique_ptr<GetEarliestMsgStoretimeResponseHeader> header(new GetEarliestMsgStoretimeResponseHeader());
  header->timestamp = std::stoll(extFields.at("timestamp"));
  return header.release();
}

void GetEarliestMsgStoretimeResponseHeader::SetDeclaredFieldOfCommandHeader(
    std::map<std::string, std::string>& requestMap) {
  // TODO: unnecessary
  requestMap.emplace("timestamp", UtilAll::to_string(timestamp));
}

//######################################
// GetConsumerListByGroupRequestHeader
//######################################

void GetConsumerListByGroupRequestHeader::Encode(Json::Value& extFields) {
  extFields["consumerGroup"] = consumerGroup;
}

void GetConsumerListByGroupRequestHeader::SetDeclaredFieldOfCommandHeader(
    std::map<std::string, std::string>& requestMap) {
  requestMap.emplace("consumerGroup", consumerGroup);
}

//######################################
// QueryConsumerOffsetRequestHeader
//######################################

void QueryConsumerOffsetRequestHeader::Encode(Json::Value& extFields) {
  extFields["consumerGroup"] = consumerGroup;
  extFields["topic"] = topic;
  extFields["queueId"] = UtilAll::to_string(queueId);
}

void QueryConsumerOffsetRequestHeader::SetDeclaredFieldOfCommandHeader(std::map<std::string, std::string>& requestMap) {
  requestMap.emplace("consumerGroup", consumerGroup);
  requestMap.emplace("topic", topic);
  requestMap.emplace("queueId", UtilAll::to_string(queueId));
}

//######################################
// QueryConsumerOffsetResponseHeader
//######################################

QueryConsumerOffsetResponseHeader* QueryConsumerOffsetResponseHeader::Decode(
    std::map<std::string, std::string>& extFields) {
  std::unique_ptr<QueryConsumerOffsetResponseHeader> header(new QueryConsumerOffsetResponseHeader());
  header->offset = std::stoll(extFields.at("offset"));
  return header.release();
}

void QueryConsumerOffsetResponseHeader::SetDeclaredFieldOfCommandHeader(
    std::map<std::string, std::string>& requestMap) {
  // TODO: unnecessary
  requestMap.emplace("offset", UtilAll::to_string(offset));
}

//######################################
// UpdateConsumerOffsetRequestHeader
//######################################

void UpdateConsumerOffsetRequestHeader::Encode(Json::Value& extFields) {
  extFields["consumerGroup"] = consumerGroup;
  extFields["topic"] = topic;
  extFields["queueId"] = UtilAll::to_string(queueId);
  extFields["commitOffset"] = UtilAll::to_string(commitOffset);
}

void UpdateConsumerOffsetRequestHeader::SetDeclaredFieldOfCommandHeader(
    std::map<std::string, std::string>& requestMap) {
  requestMap.emplace("consumerGroup", consumerGroup);
  requestMap.emplace("topic", topic);
  requestMap.emplace("queueId", UtilAll::to_string(queueId));
  requestMap.emplace("commitOffset", UtilAll::to_string(commitOffset));
}

//######################################
// ConsumerSendMsgBackRequestHeader
//######################################

void ConsumerSendMsgBackRequestHeader::Encode(Json::Value& extFields) {
  extFields["offset"] = UtilAll::to_string(offset);
  extFields["group"] = group;
  extFields["delayLevel"] = UtilAll::to_string(delayLevel);
  if (!originMsgId.empty()) {
    extFields["originMsgId"] = originMsgId;
  }
  if (!originTopic.empty()) {
    extFields["originTopic"] = originTopic;
  }
  extFields["unitMode"] = UtilAll::to_string(unitMode);
  if (maxReconsumeTimes != -1) {
    extFields["maxReconsumeTimes"] = UtilAll::to_string(maxReconsumeTimes);
  }
}

void ConsumerSendMsgBackRequestHeader::SetDeclaredFieldOfCommandHeader(std::map<std::string, std::string>& requestMap) {
  requestMap.emplace("offset", UtilAll::to_string(offset));
  requestMap.emplace("group", group);
  requestMap.emplace("delayLevel", UtilAll::to_string(delayLevel));
  if (!originMsgId.empty()) {
    requestMap.emplace("originMsgId", originMsgId);
  }
  if (!originTopic.empty()) {
    requestMap.emplace("originTopic", originTopic);
  }
  requestMap.emplace("unitMode", UtilAll::to_string(unitMode));
  if (maxReconsumeTimes != -1) {
    requestMap.emplace("maxReconsumeTimes", UtilAll::to_string(maxReconsumeTimes));
  }
}

//######################################
// GetConsumerListByGroupResponseBody
//######################################

GetConsumerListByGroupResponseBody* GetConsumerListByGroupResponseBody::Decode(const ByteArray& bodyData) {
  Json::Value root = RemotingSerializable::fromJson(bodyData);
  auto& ids = root["consumerIdList"];
  std::unique_ptr<GetConsumerListByGroupResponseBody> body(new GetConsumerListByGroupResponseBody());
  for (unsigned int i = 0; i < ids.size(); i++) {
    body->consumerIdList.push_back(ids[i].asString());
  }
  return body.release();
}

void GetConsumerListByGroupResponseBody::SetDeclaredFieldOfCommandHeader(
    std::map<std::string, std::string>& requestMap) {}

//######################################
// ResetOffsetRequestHeader
//######################################

ResetOffsetRequestHeader* ResetOffsetRequestHeader::Decode(std::map<std::string, std::string>& extFields) {
  std::unique_ptr<ResetOffsetRequestHeader> header(new ResetOffsetRequestHeader());
  header->topic = extFields.at("topic");
  header->group = extFields.at("group");
  header->timestamp = std::stoll(extFields.at("timestamp"));
  header->isForce = UtilAll::stob(extFields.at("isForce"));
  LOG_INFO_NEW("topic:{}, group:{}, timestamp:{}, isForce:{}", header->topic, header->group, header->timestamp,
               header->isForce);
  return header.release();
}

void ResetOffsetRequestHeader::setTopic(const std::string& tmp) {
  topic = tmp;
}

void ResetOffsetRequestHeader::setGroup(const std::string& tmp) {
  group = tmp;
}

void ResetOffsetRequestHeader::setTimeStamp(const int64_t& tmp) {
  timestamp = tmp;
}

void ResetOffsetRequestHeader::setForceFlag(const bool& tmp) {
  isForce = tmp;
}

const std::string& ResetOffsetRequestHeader::getTopic() const {
  return topic;
}

const std::string& ResetOffsetRequestHeader::getGroup() const {
  return group;
}

const int64_t ResetOffsetRequestHeader::getTimeStamp() const {
  return timestamp;
}

const bool ResetOffsetRequestHeader::getForceFlag() const {
  return isForce;
}

//######################################
// GetConsumerRunningInfoRequestHeader
//######################################

GetConsumerRunningInfoRequestHeader* GetConsumerRunningInfoRequestHeader::Decode(
    std::map<std::string, std::string>& extFields) {
  std::unique_ptr<GetConsumerRunningInfoRequestHeader> header(new GetConsumerRunningInfoRequestHeader());
  header->consumerGroup = extFields.at("consumerGroup");
  header->clientId = extFields.at("clientId");
  header->jstackEnable = UtilAll::stob(extFields.at("jstackEnable"));
  LOG_INFO("consumerGroup:%s, clientId:%s,  jstackEnable:%d", header->consumerGroup.c_str(), header->clientId.c_str(),
           header->jstackEnable);
  return header.release();
}

void GetConsumerRunningInfoRequestHeader::Encode(Json::Value& extFields) {
  extFields["consumerGroup"] = consumerGroup;
  extFields["clientId"] = clientId;
  extFields["jstackEnable"] = UtilAll::to_string(jstackEnable);
}

void GetConsumerRunningInfoRequestHeader::SetDeclaredFieldOfCommandHeader(
    std::map<std::string, std::string>& requestMap) {
  requestMap.emplace("consumerGroup", consumerGroup);
  requestMap.emplace("clientId", clientId);
  requestMap.emplace("jstackEnable", UtilAll::to_string(jstackEnable));
}

const std::string& GetConsumerRunningInfoRequestHeader::getConsumerGroup() const {
  return consumerGroup;
}

void GetConsumerRunningInfoRequestHeader::setConsumerGroup(const std::string& consumerGroup) {
  this->consumerGroup = consumerGroup;
}

const std::string& GetConsumerRunningInfoRequestHeader::getClientId() const {
  return clientId;
}

void GetConsumerRunningInfoRequestHeader::setClientId(const std::string& clientId) {
  this->clientId = clientId;
}

const bool GetConsumerRunningInfoRequestHeader::isJstackEnable() const {
  return jstackEnable;
}

void GetConsumerRunningInfoRequestHeader::setJstackEnable(const bool& jstackEnable) {
  this->jstackEnable = jstackEnable;
}

//######################################
// NotifyConsumerIdsChangedRequestHeader
//######################################

NotifyConsumerIdsChangedRequestHeader* NotifyConsumerIdsChangedRequestHeader::Decode(
    std::map<std::string, std::string>& extFields) {
  std::unique_ptr<NotifyConsumerIdsChangedRequestHeader> header(new NotifyConsumerIdsChangedRequestHeader());
  header->consumerGroup = extFields.at("consumerGroup");
  return header.release();
}

const std::string& NotifyConsumerIdsChangedRequestHeader::getConsumerGroup() const {
  return consumerGroup;
}

void NotifyConsumerIdsChangedRequestHeader::setConsumerGroup(const std::string& consumerGroup) {
  this->consumerGroup = consumerGroup;
}

}  // namespace rocketmq
