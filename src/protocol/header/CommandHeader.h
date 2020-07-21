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
#ifndef ROCKETMQ_PROTOCOL_COMMANDHEADER_H_
#define ROCKETMQ_PROTOCOL_COMMANDHEADER_H_

#include <vector>  // std::vector

#include "ByteArray.h"
#include "CommandCustomHeader.h"

namespace rocketmq {

class GetRouteInfoRequestHeader : public CommandCustomHeader {
 public:
  GetRouteInfoRequestHeader(const std::string& _topic) : topic(_topic) {}

  void Encode(Json::Value& extFields) override;
  void SetDeclaredFieldOfCommandHeader(std::map<std::string, std::string>& requestMap) override;

 private:
  std::string topic;
};

class UnregisterClientRequestHeader : public CommandCustomHeader {
 public:
  UnregisterClientRequestHeader(std::string cID, std::string proGroup, std::string conGroup)
      : clientID(cID), producerGroup(proGroup), consumerGroup(conGroup) {}

  void Encode(Json::Value& extFields) override;
  void SetDeclaredFieldOfCommandHeader(std::map<std::string, std::string>& requestMap) override;

 private:
  std::string clientID;
  std::string producerGroup;  // nullable
  std::string consumerGroup;  // nullable
};

class CreateTopicRequestHeader : public CommandCustomHeader {
 public:
  CreateTopicRequestHeader() : readQueueNums(0), writeQueueNums(0), perm(0), topicSysFlag(-1), order(false) {}

  void Encode(Json::Value& extFields) override;
  void SetDeclaredFieldOfCommandHeader(std::map<std::string, std::string>& requestMap) override;

 public:
  std::string topic;
  std::string defaultTopic;
  int32_t readQueueNums;
  int32_t writeQueueNums;
  int32_t perm;
  std::string topicFilterType;
  int32_t topicSysFlag;  // nullable
  bool order;
};

class CheckTransactionStateRequestHeader : public CommandCustomHeader {
 public:
  CheckTransactionStateRequestHeader() : tranStateTableOffset(0), commitLogOffset(0) {}

  static CheckTransactionStateRequestHeader* Decode(std::map<std::string, std::string>& extFields);
  void SetDeclaredFieldOfCommandHeader(std::map<std::string, std::string>& requestMap) override;
  std::string toString() const;

 public:
  int64_t tranStateTableOffset;
  int64_t commitLogOffset;
  std::string msgId;          // nullable
  std::string transactionId;  // nullable
  std::string offsetMsgId;    // nullable
};

class EndTransactionRequestHeader : public CommandCustomHeader {
 public:
  EndTransactionRequestHeader()
      : tranStateTableOffset(0), commitLogOffset(0), commitOrRollback(0), fromTransactionCheck(false) {}

  void Encode(Json::Value& extFields) override;
  void SetDeclaredFieldOfCommandHeader(std::map<std::string, std::string>& requestMap) override;
  std::string toString() const;

 public:
  std::string producerGroup;
  int64_t tranStateTableOffset;
  int64_t commitLogOffset;
  int32_t commitOrRollback;
  bool fromTransactionCheck;  // nullable
  std::string msgId;
  std::string transactionId;  // nullable
};

class SendMessageRequestHeader : public CommandCustomHeader {
 public:
  SendMessageRequestHeader()
      : defaultTopicQueueNums(0),
        queueId(0),
        sysFlag(0),
        bornTimestamp(0),
        flag(0),
        reconsumeTimes(-1),
        unitMode(false),
        batch(false),
        maxReconsumeTimes(1) {}

  void Encode(Json::Value& extFields) override;
  void SetDeclaredFieldOfCommandHeader(std::map<std::string, std::string>& requestMap) override;

  int getReconsumeTimes();
  void setReconsumeTimes(int _reconsumeTimes);

 public:
  std::string producerGroup;
  std::string topic;
  std::string defaultTopic;
  int32_t defaultTopicQueueNums;
  int32_t queueId;
  int32_t sysFlag;
  int64_t bornTimestamp;
  int32_t flag;
  std::string properties;     // nullable
  int32_t reconsumeTimes;     // nullable
  bool unitMode;              // nullable
  bool batch;                 // nullable
  int32_t maxReconsumeTimes;  // nullable
};

class SendMessageRequestHeaderV2 : public CommandCustomHeader {
 public:
  static SendMessageRequestHeaderV2* createSendMessageRequestHeaderV2(SendMessageRequestHeader* v1);

  void Encode(Json::Value& outData) override;
  void SetDeclaredFieldOfCommandHeader(std::map<std::string, std::string>& requestMap) override;

 private:
  SendMessageRequestHeaderV2() {}

 public:
  std::string a;  // producerGroup
  std::string b;  // topic
  std::string c;  // defaultTopic
  int32_t d;      // defaultTopicQueueNums
  int32_t e;      // queueId
  int32_t f;      // sysFlag
  int64_t g;      // bornTimestamp
  int32_t h;      // flag
  std::string i;  // nullable, properties
  int32_t j;      // nullable, reconsumeTimes
  bool k;         // nullable, unitMode
  int32_t l;      // nullable, maxReconsumeTimes
  bool m;         // nullable, batch
};

class SendMessageResponseHeader : public CommandCustomHeader {
 public:
  SendMessageResponseHeader() : queueId(0), queueOffset(0) {}

  static SendMessageResponseHeader* Decode(std::map<std::string, std::string>& extFields);
  void SetDeclaredFieldOfCommandHeader(std::map<std::string, std::string>& requestMap) override;

 public:
  std::string msgId;
  int32_t queueId;
  int64_t queueOffset;
  std::string transactionId;  // nullable
};

class PullMessageRequestHeader : public CommandCustomHeader {
 public:
  PullMessageRequestHeader()
      : queueId(0),
        queueOffset(0),
        maxMsgNums(0),
        sysFlag(0),
        commitOffset(0),
        suspendTimeoutMillis(0),
        subVersion(0) {}

  void Encode(Json::Value& extFields) override;
  void SetDeclaredFieldOfCommandHeader(std::map<std::string, std::string>& requestMap) override;

 public:
  std::string consumerGroup;
  std::string topic;
  int32_t queueId;
  int64_t queueOffset;
  int32_t maxMsgNums;
  int32_t sysFlag;
  int64_t commitOffset;
  int64_t suspendTimeoutMillis;
  std::string subscription;  // nullable
  int64_t subVersion;
  std::string expressionType;  // nullable
};

class PullMessageResponseHeader : public CommandCustomHeader {
 public:
  PullMessageResponseHeader() : suggestWhichBrokerId(0), nextBeginOffset(0), minOffset(0), maxOffset(0) {}

  static PullMessageResponseHeader* Decode(std::map<std::string, std::string>& extFields);
  void SetDeclaredFieldOfCommandHeader(std::map<std::string, std::string>& requestMap) override;

 public:
  int64_t suggestWhichBrokerId;
  int64_t nextBeginOffset;
  int64_t minOffset;
  int64_t maxOffset;
};

class GetConsumerListByGroupResponseHeader : public CommandCustomHeader {
 public:
  void Encode(Json::Value& extFields) override;
  void SetDeclaredFieldOfCommandHeader(std::map<std::string, std::string>& requestMap) override;
};

class GetMinOffsetRequestHeader : public CommandCustomHeader {
 public:
  GetMinOffsetRequestHeader() : queueId(0) {}

  void Encode(Json::Value& extFields) override;
  void SetDeclaredFieldOfCommandHeader(std::map<std::string, std::string>& requestMap) override;

 public:
  std::string topic;
  int32_t queueId;
};

class GetMinOffsetResponseHeader : public CommandCustomHeader {
 public:
  GetMinOffsetResponseHeader() : offset(0) {}

  static GetMinOffsetResponseHeader* Decode(std::map<std::string, std::string>& extFields);
  void SetDeclaredFieldOfCommandHeader(std::map<std::string, std::string>& requestMap) override;

 public:
  int64_t offset;
};

class GetMaxOffsetRequestHeader : public CommandCustomHeader {
 public:
  GetMaxOffsetRequestHeader() : queueId(0) {}

  void Encode(Json::Value& extFields) override;
  void SetDeclaredFieldOfCommandHeader(std::map<std::string, std::string>& requestMap) override;

 public:
  std::string topic;
  int32_t queueId;
};

class GetMaxOffsetResponseHeader : public CommandCustomHeader {
 public:
  GetMaxOffsetResponseHeader() : offset(0) {}

  static GetMaxOffsetResponseHeader* Decode(std::map<std::string, std::string>& extFields);
  void SetDeclaredFieldOfCommandHeader(std::map<std::string, std::string>& requestMap) override;

 public:
  int64_t offset;
};

class SearchOffsetRequestHeader : public CommandCustomHeader {
 public:
  SearchOffsetRequestHeader() : queueId(0), timestamp(0) {}

  void Encode(Json::Value& extFields) override;
  void SetDeclaredFieldOfCommandHeader(std::map<std::string, std::string>& requestMap) override;

 public:
  std::string topic;
  int32_t queueId;
  int64_t timestamp;
};

class SearchOffsetResponseHeader : public CommandCustomHeader {
 public:
  SearchOffsetResponseHeader() : offset(0) {}

  static SearchOffsetResponseHeader* Decode(std::map<std::string, std::string>& extFields);
  void SetDeclaredFieldOfCommandHeader(std::map<std::string, std::string>& requestMap) override;

 public:
  int64_t offset;
};

class ViewMessageRequestHeader : public CommandCustomHeader {
 public:
  ViewMessageRequestHeader() : offset(0) {}

  void Encode(Json::Value& extFields) override;
  void SetDeclaredFieldOfCommandHeader(std::map<std::string, std::string>& requestMap) override;

 public:
  int64_t offset;
};

class GetEarliestMsgStoretimeRequestHeader : public CommandCustomHeader {
 public:
  GetEarliestMsgStoretimeRequestHeader() : queueId(0) {}

  void Encode(Json::Value& extFields) override;
  void SetDeclaredFieldOfCommandHeader(std::map<std::string, std::string>& requestMap) override;

 public:
  std::string topic;
  int32_t queueId;
};

class GetEarliestMsgStoretimeResponseHeader : public CommandCustomHeader {
 public:
  GetEarliestMsgStoretimeResponseHeader() : timestamp(0) {}

  static GetEarliestMsgStoretimeResponseHeader* Decode(std::map<std::string, std::string>& extFields);
  void SetDeclaredFieldOfCommandHeader(std::map<std::string, std::string>& requestMap) override;

 public:
  int64_t timestamp;
};

class GetConsumerListByGroupRequestHeader : public CommandCustomHeader {
 public:
  void Encode(Json::Value& extFields) override;
  void SetDeclaredFieldOfCommandHeader(std::map<std::string, std::string>& requestMap) override;

 public:
  std::string consumerGroup;
};

class QueryConsumerOffsetRequestHeader : public CommandCustomHeader {
 public:
  QueryConsumerOffsetRequestHeader() : queueId(0) {}

  void Encode(Json::Value& extFields) override;
  void SetDeclaredFieldOfCommandHeader(std::map<std::string, std::string>& requestMap) override;

 public:
  std::string consumerGroup;
  std::string topic;
  int32_t queueId;
};

class QueryConsumerOffsetResponseHeader : public CommandCustomHeader {
 public:
  QueryConsumerOffsetResponseHeader() : offset(0) {}

  static QueryConsumerOffsetResponseHeader* Decode(std::map<std::string, std::string>& extFields);
  void SetDeclaredFieldOfCommandHeader(std::map<std::string, std::string>& requestMap) override;

 public:
  int64_t offset;
};

class UpdateConsumerOffsetRequestHeader : public CommandCustomHeader {
 public:
  UpdateConsumerOffsetRequestHeader() : queueId(0), commitOffset(0) {}

  void Encode(Json::Value& extFields) override;
  void SetDeclaredFieldOfCommandHeader(std::map<std::string, std::string>& requestMap) override;

 public:
  std::string consumerGroup;
  std::string topic;
  int32_t queueId;
  int64_t commitOffset;
};

class ConsumerSendMsgBackRequestHeader : public CommandCustomHeader {
 public:
  ConsumerSendMsgBackRequestHeader() : offset(0), delayLevel(0), unitMode(false), maxReconsumeTimes(-1) {}

  void Encode(Json::Value& extFields) override;
  void SetDeclaredFieldOfCommandHeader(std::map<std::string, std::string>& requestMap) override;

 public:
  int64_t offset;
  std::string group;
  int32_t delayLevel;
  std::string originMsgId;  // nullable
  std::string originTopic;  // nullable
  bool unitMode;
  int32_t maxReconsumeTimes;  // nullable
};

class GetConsumerListByGroupResponseBody : public CommandCustomHeader {
 public:
  static GetConsumerListByGroupResponseBody* Decode(const ByteArray& bodyData);
  void SetDeclaredFieldOfCommandHeader(std::map<std::string, std::string>& requestMap) override;

 public:
  std::vector<std::string> consumerIdList;
};

class ResetOffsetRequestHeader : public CommandCustomHeader {
 public:
  ResetOffsetRequestHeader() : timestamp(0), isForce(false) {}

  static ResetOffsetRequestHeader* Decode(std::map<std::string, std::string>& extFields);

  const std::string& getTopic() const;
  void setTopic(const std::string& tmp);

  const std::string& getGroup() const;
  void setGroup(const std::string& tmp);

  const int64_t getTimeStamp() const;
  void setTimeStamp(const int64_t& tmp);

  const bool getForceFlag() const;
  void setForceFlag(const bool& tmp);

 private:
  std::string topic;
  std::string group;
  int64_t timestamp;
  bool isForce;
};

class GetConsumerRunningInfoRequestHeader : public CommandCustomHeader {
 public:
  GetConsumerRunningInfoRequestHeader() : jstackEnable(false) {}

  static GetConsumerRunningInfoRequestHeader* Decode(std::map<std::string, std::string>& extFields);
  void Encode(Json::Value& extFields) override;
  void SetDeclaredFieldOfCommandHeader(std::map<std::string, std::string>& requestMap) override;

  const std::string& getConsumerGroup() const;
  void setConsumerGroup(const std::string& consumerGroup);

  const std::string& getClientId() const;
  void setClientId(const std::string& clientId);

  const bool isJstackEnable() const;
  void setJstackEnable(const bool& jstackEnable);

 private:
  std::string consumerGroup;
  std::string clientId;
  bool jstackEnable;  // nullable
};

class NotifyConsumerIdsChangedRequestHeader : public CommandCustomHeader {
 public:
  static NotifyConsumerIdsChangedRequestHeader* Decode(std::map<std::string, std::string>& extFields);

  const std::string& getConsumerGroup() const;
  void setConsumerGroup(const std::string& tmp);

 private:
  std::string consumerGroup;
};

}  // namespace rocketmq

#endif  // ROCKETMQ_PROTOCOL_COMMANDHEADER_H_
