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
#include <cstdlib>
#include <sstream>
#include "Logging.h"
#include "UtilAll.h"

namespace rocketmq {
//<!************************************************************************
void GetRouteInfoRequestHeader::Encode(Json::Value& outData) {
  outData["topic"] = topic;
}

void GetRouteInfoRequestHeader::SetDeclaredFieldOfCommandHeader(map<string, string>& requestMap) {
  requestMap.insert(pair<string, string>("topic", topic));
}
//<!***************************************************************************
void UnregisterClientRequestHeader::Encode(Json::Value& outData) {
  outData["clientID"] = clientID;
  outData["producerGroup"] = producerGroup;
  outData["consumerGroup"] = consumerGroup;
}

void UnregisterClientRequestHeader::SetDeclaredFieldOfCommandHeader(map<string, string>& requestMap) {
  requestMap.insert(pair<string, string>("clientID", clientID));
  requestMap.insert(pair<string, string>("producerGroup", producerGroup));
  requestMap.insert(pair<string, string>("consumerGroup", consumerGroup));
}
//<!************************************************************************
void CreateTopicRequestHeader::Encode(Json::Value& outData) {
  outData["topic"] = topic;
  outData["defaultTopic"] = defaultTopic;
  outData["readQueueNums"] = readQueueNums;
  outData["writeQueueNums"] = writeQueueNums;
  outData["perm"] = perm;
  outData["topicFilterType"] = topicFilterType;
}
void CreateTopicRequestHeader::SetDeclaredFieldOfCommandHeader(map<string, string>& requestMap) {
  requestMap.insert(pair<string, string>("topic", topic));
  requestMap.insert(pair<string, string>("defaultTopic", defaultTopic));
  requestMap.insert(pair<string, string>("readQueueNums", UtilAll::to_string(readQueueNums)));
  requestMap.insert(pair<string, string>("writeQueueNums", UtilAll::to_string(writeQueueNums)));
  requestMap.insert(pair<string, string>("perm", UtilAll::to_string(perm)));
  requestMap.insert(pair<string, string>("topicFilterType", topicFilterType));
}

void CheckTransactionStateRequestHeader::Encode(Json::Value& outData) {}

CommandHeader* CheckTransactionStateRequestHeader::Decode(Json::Value& ext) {
  CheckTransactionStateRequestHeader* h = new CheckTransactionStateRequestHeader();
  Json::Value& tempValue = ext["msgId"];
  if (tempValue.isString()) {
    h->m_msgId = tempValue.asString();
  }

  tempValue = ext["transactionId"];
  if (tempValue.isString()) {
    h->m_transactionId = tempValue.asString();
  }

  tempValue = ext["offsetMsgId"];
  if (tempValue.isString()) {
    h->m_offsetMsgId = tempValue.asString();
  }

  tempValue = ext["tranStateTableOffset"];
  if (tempValue.isString()) {
    h->m_tranStateTableOffset = UtilAll::str2ll(tempValue.asCString());
  }

  tempValue = ext["commitLogOffset"];
  if (tempValue.isString()) {
    h->m_commitLogOffset = UtilAll::str2ll(tempValue.asCString());
  }

  return h;
}

void CheckTransactionStateRequestHeader::SetDeclaredFieldOfCommandHeader(map<string, string>& requestMap) {
  requestMap.insert(pair<string, string>("msgId", m_msgId));
  requestMap.insert(pair<string, string>("transactionId", m_transactionId));
  requestMap.insert(pair<string, string>("offsetMsgId", m_offsetMsgId));
  requestMap.insert(pair<string, string>("commitLogOffset", UtilAll::to_string(m_commitLogOffset)));
  requestMap.insert(pair<string, string>("tranStateTableOffset", UtilAll::to_string(m_tranStateTableOffset)));
}

std::string CheckTransactionStateRequestHeader::toString() {
  stringstream ss;
  ss << "CheckTransactionStateRequestHeader:";
  ss << " msgId:" << m_msgId;
  ss << " transactionId:" << m_transactionId;
  ss << " offsetMsgId:" << m_offsetMsgId;
  ss << " commitLogOffset:" << m_commitLogOffset;
  ss << " tranStateTableOffset:" << m_tranStateTableOffset;
  return ss.str();
}

void EndTransactionRequestHeader::Encode(Json::Value& outData) {
  outData["msgId"] = m_msgId;
  outData["transactionId"] = m_transactionId;
  outData["producerGroup"] = m_producerGroup;
  outData["tranStateTableOffset"] = UtilAll::to_string(m_tranStateTableOffset);
  outData["commitLogOffset"] = UtilAll::to_string(m_commitLogOffset);
  outData["commitOrRollback"] = UtilAll::to_string(m_commitOrRollback);
  outData["fromTransactionCheck"] = UtilAll::to_string(m_fromTransactionCheck);
}

void EndTransactionRequestHeader::SetDeclaredFieldOfCommandHeader(map<string, string>& requestMap) {
  requestMap.insert(pair<string, string>("msgId", m_msgId));
  requestMap.insert(pair<string, string>("transactionId", m_transactionId));
  requestMap.insert(pair<string, string>("producerGroup", m_producerGroup));
  requestMap.insert(pair<string, string>("tranStateTableOffset", UtilAll::to_string(m_tranStateTableOffset)));
  requestMap.insert(pair<string, string>("commitLogOffset", UtilAll::to_string(m_commitLogOffset)));
  requestMap.insert(pair<string, string>("commitOrRollback", UtilAll::to_string(m_commitOrRollback)));
  requestMap.insert(pair<string, string>("fromTransactionCheck", UtilAll::to_string(m_fromTransactionCheck)));
}

std::string EndTransactionRequestHeader::toString() {
  stringstream ss;
  ss << "EndTransactionRequestHeader:";
  ss << " m_msgId:" << m_msgId;
  ss << " m_transactionId:" << m_transactionId;
  ss << " m_producerGroup:" << m_producerGroup;
  ss << " m_tranStateTableOffset:" << m_tranStateTableOffset;
  ss << " m_commitLogOffset:" << m_commitLogOffset;
  ss << " m_commitOrRollback:" << m_commitOrRollback;
  ss << " m_fromTransactionCheck:" << m_fromTransactionCheck;
  return ss.str();
}

//<!************************************************************************
void SendMessageRequestHeader::Encode(Json::Value& outData) {
  outData["producerGroup"] = producerGroup;
  outData["topic"] = topic;
  outData["defaultTopic"] = defaultTopic;
  outData["defaultTopicQueueNums"] = defaultTopicQueueNums;
  outData["queueId"] = queueId;
  outData["sysFlag"] = sysFlag;
  outData["bornTimestamp"] = UtilAll::to_string(bornTimestamp);
  outData["flag"] = flag;
  outData["properties"] = properties;
  outData["reconsumeTimes"] = UtilAll::to_string(reconsumeTimes);
  outData["unitMode"] = UtilAll::to_string(unitMode);
  outData["batch"] = UtilAll::to_string(batch);
}

void SendMessageRequestHeader::SetDeclaredFieldOfCommandHeader(map<string, string>& requestMap) {
  LOG_DEBUG(
      "SendMessageRequestHeader producerGroup is:%s,topic is:%s, defaulttopic "
      "is:%s, properties is:%s,UtilAll::to_string( defaultTopicQueueNums) "
      "is:%s,UtilAll::to_string( queueId):%s, UtilAll::to_string( sysFlag) "
      "is:%s, UtilAll::to_string( bornTimestamp) is:%s,UtilAll::to_string( "
      "flag) is:%s",
      producerGroup.c_str(), topic.c_str(), defaultTopic.c_str(), properties.c_str(),
      UtilAll::to_string(defaultTopicQueueNums).c_str(), UtilAll::to_string(queueId).c_str(),
      UtilAll::to_string(sysFlag).c_str(), UtilAll::to_string(bornTimestamp).c_str(), UtilAll::to_string(flag).c_str());

  requestMap.insert(pair<string, string>("producerGroup", producerGroup));
  requestMap.insert(pair<string, string>("topic", topic));
  requestMap.insert(pair<string, string>("defaultTopic", defaultTopic));
  requestMap.insert(pair<string, string>("defaultTopicQueueNums", UtilAll::to_string(defaultTopicQueueNums)));
  requestMap.insert(pair<string, string>("queueId", UtilAll::to_string(queueId)));
  requestMap.insert(pair<string, string>("sysFlag", UtilAll::to_string(sysFlag)));
  requestMap.insert(pair<string, string>("bornTimestamp", UtilAll::to_string(bornTimestamp)));
  requestMap.insert(pair<string, string>("flag", UtilAll::to_string(flag)));
  requestMap.insert(pair<string, string>("properties", properties));
  requestMap.insert(pair<string, string>("reconsumeTimes", UtilAll::to_string(reconsumeTimes)));
  requestMap.insert(pair<string, string>("unitMode", UtilAll::to_string(unitMode)));
  requestMap.insert(pair<string, string>("batch", UtilAll::to_string(batch)));
}

//<!************************************************************************
void SendMessageRequestHeaderV2::Encode(Json::Value& outData) {
  outData["a"] = a;                      // string producerGroup;
  outData["b"] = b;                      // string topic;
  outData["c"] = c;                      // string defaultTopic;
  outData["d"] = d;                      // int defaultTopicQueueNums;
  outData["e"] = e;                      // int queueId;
  outData["f"] = f;                      // int sysFlag;
  outData["g"] = UtilAll::to_string(g);  // int64 bornTimestamp;
  outData["h"] = h;                      // int flag;
  outData["i"] = i;                      // string properties;
  outData["j"] = UtilAll::to_string(j);  // int reconsumeTimes;
  outData["k"] = UtilAll::to_string(k);  // bool unitMode;
  outData["l"] = l;                      // int consumeRetryTimes;
  outData["m"] = UtilAll::to_string(m);  // bool batch;
}

void SendMessageRequestHeaderV2::SetDeclaredFieldOfCommandHeader(map<string, string>& requestMap) {
  LOG_DEBUG(
      "SendMessageRequestHeaderV2 producerGroup is:%s,topic is:%s, defaulttopic "
      "is:%s, properties is:%s,UtilAll::to_string( defaultTopicQueueNums) "
      "is:%s,UtilAll::to_string( queueId):%s, UtilAll::to_string( sysFlag) "
      "is:%s, UtilAll::to_string( bornTimestamp) is:%s,UtilAll::to_string( "
      "flag) is:%s,UtilAll::to_string( reconsumeTimes) is:%s,UtilAll::to_string( unitMode) is:%s,UtilAll::to_string( "
      "batch) is:%s",
      a.c_str(), b.c_str(), c.c_str(), i.c_str(), UtilAll::to_string(d).c_str(), UtilAll::to_string(e).c_str(),
      UtilAll::to_string(f).c_str(), UtilAll::to_string(g).c_str(), UtilAll::to_string(g).c_str(),
      UtilAll::to_string(j).c_str(), UtilAll::to_string(k).c_str(), UtilAll::to_string(m).c_str());

  requestMap.insert(pair<string, string>("a", a));
  requestMap.insert(pair<string, string>("b", b));
  requestMap.insert(pair<string, string>("c", c));
  requestMap.insert(pair<string, string>("d", UtilAll::to_string(d)));
  requestMap.insert(pair<string, string>("e", UtilAll::to_string(e)));
  requestMap.insert(pair<string, string>("f", UtilAll::to_string(f)));
  requestMap.insert(pair<string, string>("g", UtilAll::to_string(g)));
  requestMap.insert(pair<string, string>("h", UtilAll::to_string(h)));
  requestMap.insert(pair<string, string>("i", i));
  requestMap.insert(pair<string, string>("j", UtilAll::to_string(j)));
  requestMap.insert(pair<string, string>("k", UtilAll::to_string(k)));
  requestMap.insert(pair<string, string>("l", UtilAll::to_string(l)));
  requestMap.insert(pair<string, string>("m", UtilAll::to_string(m)));
}
void SendMessageRequestHeaderV2::CreateSendMessageRequestHeaderV1(SendMessageRequestHeader& v1) {
  v1.producerGroup = a;
  v1.topic = b;
  v1.defaultTopic = c;
  v1.defaultTopicQueueNums = d;
  v1.queueId = e;
  v1.sysFlag = f;
  v1.bornTimestamp = g;
  v1.flag = h;
  v1.properties = i;
  v1.reconsumeTimes = j;
  v1.unitMode = k;
  v1.consumeRetryTimes = l;
  v1.batch = m;
}
//<!************************************************************************
CommandHeader* SendMessageResponseHeader::Decode(Json::Value& ext) {
  SendMessageResponseHeader* h = new SendMessageResponseHeader();

  Json::Value& tempValue = ext["msgId"];
  if (tempValue.isString()) {
    h->msgId = tempValue.asString();
  }

  tempValue = ext["queueId"];
  if (tempValue.isString()) {
    h->queueId = atoi(tempValue.asCString());
  }

  tempValue = ext["queueOffset"];
  if (tempValue.isString()) {
    h->queueOffset = UtilAll::str2ll(tempValue.asCString());
  }

  tempValue = ext["transactionId"];
  if (tempValue.isString()) {
    h->transactionId = tempValue.asCString();
  }

  tempValue = ext["MSG_REGION"];
  if (tempValue.isString()) {
    h->regionId = tempValue.asCString();
  }
  return h;
}

void SendMessageResponseHeader::SetDeclaredFieldOfCommandHeader(map<string, string>& requestMap) {
  requestMap.insert(pair<string, string>("msgId", msgId));
  requestMap.insert(pair<string, string>("queueId", UtilAll::to_string(queueId)));
  requestMap.insert(pair<string, string>("queueOffset", UtilAll::to_string(queueOffset)));
  requestMap.insert(pair<string, string>("transactionId", transactionId));
  requestMap.insert(pair<string, string>("MSG_REGION", regionId));
}
//<!************************************************************************
void PullMessageRequestHeader::Encode(Json::Value& outData) {
  outData["consumerGroup"] = consumerGroup;
  outData["topic"] = topic;
  outData["queueId"] = queueId;
  outData["queueOffset"] = UtilAll::to_string(queueOffset);
  outData["maxMsgNums"] = maxMsgNums;
  outData["sysFlag"] = sysFlag;
  outData["commitOffset"] = UtilAll::to_string(commitOffset);
  outData["subVersion"] = UtilAll::to_string(subVersion);
  outData["suspendTimeoutMillis"] = UtilAll::to_string(suspendTimeoutMillis);
  outData["subscription"] = subscription;
}

void PullMessageRequestHeader::SetDeclaredFieldOfCommandHeader(map<string, string>& requestMap) {
  requestMap.insert(pair<string, string>("consumerGroup", consumerGroup));
  requestMap.insert(pair<string, string>("topic", topic));
  requestMap.insert(pair<string, string>("queueId", UtilAll::to_string(queueId)));
  requestMap.insert(pair<string, string>("queueOffset", UtilAll::to_string(queueOffset)));
  requestMap.insert(pair<string, string>("maxMsgNums", UtilAll::to_string(maxMsgNums)));
  requestMap.insert(pair<string, string>("sysFlag", UtilAll::to_string(sysFlag)));
  requestMap.insert(pair<string, string>("commitOffset", UtilAll::to_string(commitOffset)));
  requestMap.insert(pair<string, string>("subVersion", UtilAll::to_string(subVersion)));
  requestMap.insert(pair<string, string>("suspendTimeoutMillis", UtilAll::to_string(suspendTimeoutMillis)));
  requestMap.insert(pair<string, string>("subscription", subscription));
}
//<!************************************************************************
CommandHeader* PullMessageResponseHeader::Decode(Json::Value& ext) {
  PullMessageResponseHeader* h = new PullMessageResponseHeader();

  Json::Value& tempValue = ext["suggestWhichBrokerId"];
  if (tempValue.isString()) {
    h->suggestWhichBrokerId = UtilAll::str2ll(tempValue.asCString());
  }

  tempValue = ext["nextBeginOffset"];
  if (tempValue.isString()) {
    h->nextBeginOffset = UtilAll::str2ll(tempValue.asCString());
  }

  tempValue = ext["minOffset"];
  if (tempValue.isString()) {
    h->minOffset = UtilAll::str2ll(tempValue.asCString());
  }

  tempValue = ext["maxOffset"];
  if (tempValue.isString()) {
    h->maxOffset = UtilAll::str2ll(tempValue.asCString());
  }

  return h;
}

void PullMessageResponseHeader::SetDeclaredFieldOfCommandHeader(map<string, string>& requestMap) {
  requestMap.insert(pair<string, string>("suggestWhichBrokerId", UtilAll::to_string(suggestWhichBrokerId)));
  requestMap.insert(pair<string, string>("nextBeginOffset", UtilAll::to_string(nextBeginOffset)));
  requestMap.insert(pair<string, string>("minOffset", UtilAll::to_string(minOffset)));
  requestMap.insert(pair<string, string>("maxOffset", UtilAll::to_string(maxOffset)));
}
//<!************************************************************************
void GetConsumerListByGroupResponseHeader::Encode(Json::Value& outData) {
  // outData = "{}";
}

void GetConsumerListByGroupResponseHeader::SetDeclaredFieldOfCommandHeader(map<string, string>& requestMap) {}
//<!***************************************************************************
void GetMinOffsetRequestHeader::Encode(Json::Value& outData) {
  outData["topic"] = topic;
  outData["queueId"] = queueId;
}

void GetMinOffsetRequestHeader::SetDeclaredFieldOfCommandHeader(map<string, string>& requestMap) {
  requestMap.insert(pair<string, string>("topic", topic));
  requestMap.insert(pair<string, string>("queueId", UtilAll::to_string(queueId)));
}
//<!***************************************************************************
CommandHeader* GetMinOffsetResponseHeader::Decode(Json::Value& ext) {
  GetMinOffsetResponseHeader* h = new GetMinOffsetResponseHeader();

  Json::Value& tempValue = ext["offset"];
  if (tempValue.isString()) {
    h->offset = UtilAll::str2ll(tempValue.asCString());
  }
  return h;
}

void GetMinOffsetResponseHeader::SetDeclaredFieldOfCommandHeader(map<string, string>& requestMap) {
  requestMap.insert(pair<string, string>("offset", UtilAll::to_string(offset)));
}
//<!***************************************************************************
void GetMaxOffsetRequestHeader::Encode(Json::Value& outData) {
  outData["topic"] = topic;
  outData["queueId"] = queueId;
}

void GetMaxOffsetRequestHeader::SetDeclaredFieldOfCommandHeader(map<string, string>& requestMap) {
  requestMap.insert(pair<string, string>("topic", topic));
  requestMap.insert(pair<string, string>("queueId", UtilAll::to_string(queueId)));
}
//<!***************************************************************************
CommandHeader* GetMaxOffsetResponseHeader::Decode(Json::Value& ext) {
  GetMaxOffsetResponseHeader* h = new GetMaxOffsetResponseHeader();

  Json::Value& tempValue = ext["offset"];
  if (tempValue.isString()) {
    h->offset = UtilAll::str2ll(tempValue.asCString());
  }
  return h;
}

void GetMaxOffsetResponseHeader::SetDeclaredFieldOfCommandHeader(map<string, string>& requestMap) {
  requestMap.insert(pair<string, string>("offset", UtilAll::to_string(offset)));
}
//<!***************************************************************************
void SearchOffsetRequestHeader::Encode(Json::Value& outData) {
  outData["topic"] = topic;
  outData["queueId"] = queueId;
  outData["timestamp"] = UtilAll::to_string(timestamp);
}

void SearchOffsetRequestHeader::SetDeclaredFieldOfCommandHeader(map<string, string>& requestMap) {
  requestMap.insert(pair<string, string>("topic", topic));
  requestMap.insert(pair<string, string>("queueId", UtilAll::to_string(queueId)));
  requestMap.insert(pair<string, string>("timestamp", UtilAll::to_string(timestamp)));
}
//<!***************************************************************************
CommandHeader* SearchOffsetResponseHeader::Decode(Json::Value& ext) {
  SearchOffsetResponseHeader* h = new SearchOffsetResponseHeader();

  Json::Value& tempValue = ext["offset"];
  if (tempValue.isString()) {
    h->offset = UtilAll::str2ll(tempValue.asCString());
  }
  return h;
}

void SearchOffsetResponseHeader::SetDeclaredFieldOfCommandHeader(map<string, string>& requestMap) {
  requestMap.insert(pair<string, string>("offset", UtilAll::to_string(offset)));
}
//<!***************************************************************************
void ViewMessageRequestHeader::Encode(Json::Value& outData) {
  outData["offset"] = UtilAll::to_string(offset);
}

void ViewMessageRequestHeader::SetDeclaredFieldOfCommandHeader(map<string, string>& requestMap) {
  requestMap.insert(pair<string, string>("offset", UtilAll::to_string(offset)));
}
//<!***************************************************************************
void GetEarliestMsgStoretimeRequestHeader::Encode(Json::Value& outData) {
  outData["topic"] = topic;
  outData["queueId"] = queueId;
}

void GetEarliestMsgStoretimeRequestHeader::SetDeclaredFieldOfCommandHeader(map<string, string>& requestMap) {
  requestMap.insert(pair<string, string>("topic", topic));
  requestMap.insert(pair<string, string>("queueId", UtilAll::to_string(queueId)));
}
//<!***************************************************************************
CommandHeader* GetEarliestMsgStoretimeResponseHeader::Decode(Json::Value& ext) {
  GetEarliestMsgStoretimeResponseHeader* h = new GetEarliestMsgStoretimeResponseHeader();

  Json::Value& tempValue = ext["timestamp"];
  if (tempValue.isString()) {
    h->timestamp = UtilAll::str2ll(tempValue.asCString());
  }
  return h;
}

void GetEarliestMsgStoretimeResponseHeader::SetDeclaredFieldOfCommandHeader(map<string, string>& requestMap) {
  requestMap.insert(pair<string, string>("timestamp", UtilAll::to_string(timestamp)));
}
//<!***************************************************************************
void GetConsumerListByGroupRequestHeader::Encode(Json::Value& outData) {
  outData["consumerGroup"] = consumerGroup;
}

void GetConsumerListByGroupRequestHeader::SetDeclaredFieldOfCommandHeader(map<string, string>& requestMap) {
  requestMap.insert(pair<string, string>("consumerGroup", consumerGroup));
}
//<!***************************************************************************
void QueryConsumerOffsetRequestHeader::Encode(Json::Value& outData) {
  outData["consumerGroup"] = consumerGroup;
  outData["topic"] = topic;
  outData["queueId"] = queueId;
}

void QueryConsumerOffsetRequestHeader::SetDeclaredFieldOfCommandHeader(map<string, string>& requestMap) {
  requestMap.insert(pair<string, string>("consumerGroup", consumerGroup));
  requestMap.insert(pair<string, string>("topic", topic));
  requestMap.insert(pair<string, string>("queueId", UtilAll::to_string(queueId)));
}
//<!***************************************************************************
CommandHeader* QueryConsumerOffsetResponseHeader::Decode(Json::Value& ext) {
  QueryConsumerOffsetResponseHeader* h = new QueryConsumerOffsetResponseHeader();
  Json::Value& tempValue = ext["offset"];
  if (tempValue.isString()) {
    h->offset = UtilAll::str2ll(tempValue.asCString());
  }
  return h;
}

void QueryConsumerOffsetResponseHeader::SetDeclaredFieldOfCommandHeader(map<string, string>& requestMap) {
  requestMap.insert(pair<string, string>("offset", UtilAll::to_string(offset)));
}
//<!***************************************************************************
void UpdateConsumerOffsetRequestHeader::Encode(Json::Value& outData) {
  outData["consumerGroup"] = consumerGroup;
  outData["topic"] = topic;
  outData["queueId"] = queueId;
  outData["commitOffset"] = UtilAll::to_string(commitOffset);
}

void UpdateConsumerOffsetRequestHeader::SetDeclaredFieldOfCommandHeader(map<string, string>& requestMap) {
  requestMap.insert(pair<string, string>("consumerGroup", consumerGroup));
  requestMap.insert(pair<string, string>("topic", topic));
  requestMap.insert(pair<string, string>("queueId", UtilAll::to_string(queueId)));
  requestMap.insert(pair<string, string>("commitOffset", UtilAll::to_string(commitOffset)));
}
//<!***************************************************************************
void ConsumerSendMsgBackRequestHeader::Encode(Json::Value& outData) {
  outData["group"] = group;
  outData["delayLevel"] = delayLevel;
  outData["offset"] = UtilAll::to_string(offset);
  outData["unitMode"] = UtilAll::to_string(unitMode);
  outData["originMsgId"] = originMsgId;
  outData["originTopic"] = originTopic;
  outData["maxReconsumeTimes"] = maxReconsumeTimes;
}

void ConsumerSendMsgBackRequestHeader::SetDeclaredFieldOfCommandHeader(map<string, string>& requestMap) {
  requestMap.insert(pair<string, string>("group", group));
  requestMap.insert(pair<string, string>("delayLevel", UtilAll::to_string(delayLevel)));
  requestMap.insert(pair<string, string>("offset", UtilAll::to_string(offset)));
  requestMap.insert(pair<string, string>("unitMode", UtilAll::to_string(unitMode)));
  requestMap.insert(pair<string, string>("originMsgId", originMsgId));
  requestMap.insert(pair<string, string>("originTopic", originTopic));
  requestMap.insert(pair<string, string>("maxReconsumeTimes", UtilAll::to_string(maxReconsumeTimes)));
}
//<!***************************************************************************
void GetConsumerListByGroupResponseBody::Decode(const MemoryBlock* mem, vector<string>& cids) {
  cids.clear();
  //<! decode;
  const std::string pData(mem->getData(), mem->getSize());

  Json::Reader reader;
  Json::Value root;
  if (!reader.parse(pData, root)) {
    LOG_ERROR("GetConsumerListByGroupResponse error");
    return;
  }

  Json::Value ids = root["consumerIdList"];
  for (unsigned int i = 0; i < ids.size(); i++) {
    if (ids[i].isString()) {
      cids.push_back(ids[i].asString());
    }
  }
}

void GetConsumerListByGroupResponseBody::SetDeclaredFieldOfCommandHeader(map<string, string>& requestMap) {}

void ResetOffsetRequestHeader::setTopic(const string& tmp) {
  topic = tmp;
}

void ResetOffsetRequestHeader::setGroup(const string& tmp) {
  group = tmp;
}

void ResetOffsetRequestHeader::setTimeStamp(const int64& tmp) {
  timestamp = tmp;
}

void ResetOffsetRequestHeader::setForceFlag(const bool& tmp) {
  isForce = tmp;
}

const string ResetOffsetRequestHeader::getTopic() const {
  return topic;
}

const string ResetOffsetRequestHeader::getGroup() const {
  return group;
}

const int64 ResetOffsetRequestHeader::getTimeStamp() const {
  return timestamp;
}

const bool ResetOffsetRequestHeader::getForceFlag() const {
  return isForce;
}

CommandHeader* ResetOffsetRequestHeader::Decode(Json::Value& ext) {
  ResetOffsetRequestHeader* h = new ResetOffsetRequestHeader();

  Json::Value& tempValue = ext["topic"];
  if (tempValue.isString()) {
    h->topic = tempValue.asString();
  }

  tempValue = ext["group"];
  if (tempValue.isString()) {
    h->group = tempValue.asString();
  }

  tempValue = ext["timestamp"];
  if (tempValue.isString()) {
    h->timestamp = UtilAll::str2ll(tempValue.asCString());
  }

  tempValue = ext["isForce"];
  if (tempValue.isString()) {
    h->isForce = UtilAll::to_bool(tempValue.asCString());
  }
  LOG_INFO("topic:%s, group:%s, timestamp:%lld, isForce:%d", h->topic.c_str(), h->group.c_str(), h->timestamp,
           h->isForce);
  return h;
}

CommandHeader* GetConsumerRunningInfoRequestHeader::Decode(Json::Value& ext) {
  GetConsumerRunningInfoRequestHeader* h = new GetConsumerRunningInfoRequestHeader();

  Json::Value& tempValue = ext["consumerGroup"];
  if (tempValue.isString()) {
    h->consumerGroup = tempValue.asString();
  }

  tempValue = ext["clientId"];
  if (tempValue.isString()) {
    h->clientId = tempValue.asString();
  }

  tempValue = ext["jstackEnable"];
  if (tempValue.isBool()) {
    h->jstackEnable = tempValue.asBool();
  } else if (tempValue.isString()) {
    h->jstackEnable = UtilAll::to_bool(tempValue.asCString());
  }
  LOG_INFO("consumerGroup:%s, clientId:%s,  jstackEnable:%d", h->consumerGroup.c_str(), h->clientId.c_str(),
           h->jstackEnable);
  return h;
}

void GetConsumerRunningInfoRequestHeader::Encode(Json::Value& outData) {
  outData["consumerGroup"] = consumerGroup;
  outData["clientId"] = clientId;
  outData["jstackEnable"] = jstackEnable;
}

void GetConsumerRunningInfoRequestHeader::SetDeclaredFieldOfCommandHeader(map<string, string>& requestMap) {
  requestMap.insert(pair<string, string>("consumerGroup", consumerGroup));
  requestMap.insert(pair<string, string>("clientId", clientId));
  requestMap.insert(pair<string, string>("jstackEnable", UtilAll::to_string(jstackEnable)));
}

const string GetConsumerRunningInfoRequestHeader::getConsumerGroup() const {
  return consumerGroup;
}

void GetConsumerRunningInfoRequestHeader::setConsumerGroup(const string& Group) {
  consumerGroup = Group;
}

const string GetConsumerRunningInfoRequestHeader::getClientId() const {
  return clientId;
}

void GetConsumerRunningInfoRequestHeader::setClientId(const string& input_clientId) {
  clientId = input_clientId;
}

const bool GetConsumerRunningInfoRequestHeader::isJstackEnable() const {
  return jstackEnable;
}

void GetConsumerRunningInfoRequestHeader::setJstackEnable(const bool& input_jstackEnable) {
  jstackEnable = input_jstackEnable;
}

CommandHeader* NotifyConsumerIdsChangedRequestHeader::Decode(Json::Value& ext) {
  NotifyConsumerIdsChangedRequestHeader* h = new NotifyConsumerIdsChangedRequestHeader();

  Json::Value& tempValue = ext["consumerGroup"];
  if (tempValue.isString()) {
    h->consumerGroup = tempValue.asString();
  }

  return h;
}

void NotifyConsumerIdsChangedRequestHeader::setGroup(const string& tmp) {
  consumerGroup = tmp;
}
const string NotifyConsumerIdsChangedRequestHeader::getGroup() const {
  return consumerGroup;
}

//<!************************************************************************
}  // namespace rocketmq
