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

#ifndef __COMMANDCUSTOMHEADER_H__
#define __COMMANDCUSTOMHEADER_H__

#include <map>
#include <string>

#include "MQClientException.h"
#include "MessageSysFlag.h"
#include "UtilAll.h"
#include "dataBlock.h"
#include "json/json.h"

namespace rocketmq {
//<!***************************************************************************

class CommandHeader {
 public:
  virtual ~CommandHeader() {}
  virtual void Encode(Json::Value& outData) {}
  virtual void SetDeclaredFieldOfCommandHeader(std::map<std::string, std::string>& requestMap) {}
};

class CheckTransactionStateRequestHeader : public CommandHeader {
 public:
  CheckTransactionStateRequestHeader() {}
  CheckTransactionStateRequestHeader(long tableOffset,
                                     long commLogOffset,
                                     const std::string& msgid,
                                     const std::string& transactionId,
                                     const std::string& offsetMsgId)
      : m_tranStateTableOffset(tableOffset),
        m_commitLogOffset(commLogOffset),
        m_msgId(msgid),
        m_transactionId(transactionId),
        m_offsetMsgId(offsetMsgId) {}
  virtual ~CheckTransactionStateRequestHeader() {}
  virtual void Encode(Json::Value& outData);
  static CommandHeader* Decode(Json::Value& ext);
  virtual void SetDeclaredFieldOfCommandHeader(std::map<std::string, std::string>& requestMap);
  std::string toString();

 public:
  long m_tranStateTableOffset;
  long m_commitLogOffset;
  std::string m_msgId;
  std::string m_transactionId;
  std::string m_offsetMsgId;
};

class EndTransactionRequestHeader : public CommandHeader {
 public:
  EndTransactionRequestHeader() {}
  EndTransactionRequestHeader(const std::string& groupName,
                              long tableOffset,
                              long commLogOffset,
                              int commitOrRoll,
                              bool fromTransCheck,
                              const std::string& msgid,
                              const std::string& transId)
      : m_producerGroup(groupName),
        m_tranStateTableOffset(tableOffset),
        m_commitLogOffset(commLogOffset),
        m_commitOrRollback(commitOrRoll),
        m_fromTransactionCheck(fromTransCheck),
        m_msgId(msgid),
        m_transactionId(transId) {}
  virtual ~EndTransactionRequestHeader() {}
  virtual void Encode(Json::Value& outData);
  virtual void SetDeclaredFieldOfCommandHeader(std::map<std::string, std::string>& requestMap);
  std::string toString();

 public:
  std::string m_producerGroup;
  long m_tranStateTableOffset;
  long m_commitLogOffset;
  int m_commitOrRollback;
  bool m_fromTransactionCheck;
  std::string m_msgId;
  std::string m_transactionId;
};

//<!************************************************************************
class GetRouteInfoRequestHeader : public CommandHeader {
 public:
  GetRouteInfoRequestHeader(const std::string& top) : topic(top) {}
  virtual ~GetRouteInfoRequestHeader() {}
  virtual void Encode(Json::Value& outData);
  virtual void SetDeclaredFieldOfCommandHeader(std::map<std::string, std::string>& requestMap);

 private:
  std::string topic;
};

//<!************************************************************************
class UnregisterClientRequestHeader : public CommandHeader {
 public:
  UnregisterClientRequestHeader(std::string cID, std::string proGroup, std::string conGroup)
      : clientID(cID), producerGroup(proGroup), consumerGroup(conGroup) {}
  virtual ~UnregisterClientRequestHeader() {}
  virtual void Encode(Json::Value& outData);
  virtual void SetDeclaredFieldOfCommandHeader(std::map<std::string, std::string>& requestMap);

 private:
  std::string clientID;
  std::string producerGroup;
  std::string consumerGroup;
};

//<!************************************************************************
class CreateTopicRequestHeader : public CommandHeader {
 public:
  CreateTopicRequestHeader() : readQueueNums(0), writeQueueNums(0), perm(0) {}
  virtual ~CreateTopicRequestHeader() {}
  virtual void Encode(Json::Value& outData);
  virtual void SetDeclaredFieldOfCommandHeader(std::map<std::string, std::string>& requestMap);

 public:
  std::string topic;
  std::string defaultTopic;
  int readQueueNums;
  int writeQueueNums;
  int perm;
  std::string topicFilterType;
};

//<!************************************************************************
class SendMessageRequestHeader : public CommandHeader {
 public:
  SendMessageRequestHeader()
      : defaultTopicQueueNums(0),
        queueId(0),
        sysFlag(0),
        bornTimestamp(0),
        flag(0),
        reconsumeTimes(0),
        unitMode(false),
        batch(false) {}
  virtual ~SendMessageRequestHeader() {}
  virtual void Encode(Json::Value& outData);
  virtual void SetDeclaredFieldOfCommandHeader(std::map<std::string, std::string>& requestMap);
  int getReconsumeTimes();
  void setReconsumeTimes(int input_reconsumeTimes);

 public:
  std::string producerGroup;
  std::string topic;
  std::string defaultTopic;
  int defaultTopicQueueNums;
  int queueId;
  int sysFlag;
  int64 bornTimestamp;
  int flag;
  std::string properties;
  int reconsumeTimes;
  bool unitMode;
  bool batch;
};

//<!************************************************************************
class SendMessageResponseHeader : public CommandHeader {
 public:
  SendMessageResponseHeader() : queueId(0), queueOffset(0) { msgId.clear(); }
  virtual ~SendMessageResponseHeader() {}
  static CommandHeader* Decode(Json::Value& ext);
  virtual void SetDeclaredFieldOfCommandHeader(std::map<std::string, std::string>& requestMap);

 public:
  std::string msgId;
  int queueId;
  int64 queueOffset;
};

//<!************************************************************************
class PullMessageRequestHeader : public CommandHeader {
 public:
  PullMessageRequestHeader()
      : queueId(0),
        maxMsgNums(0),
        sysFlag(0),
        queueOffset(0),
        commitOffset(0),
        suspendTimeoutMillis(0),
        subVersion(0) {}
  virtual ~PullMessageRequestHeader() {}
  virtual void Encode(Json::Value& outData);
  virtual void SetDeclaredFieldOfCommandHeader(std::map<std::string, std::string>& requestMap);

 public:
  std::string consumerGroup;
  std::string topic;
  int queueId;
  int maxMsgNums;
  int sysFlag;
  std::string subscription;
  int64 queueOffset;
  int64 commitOffset;
  int64 suspendTimeoutMillis;
  int64 subVersion;
};

//<!************************************************************************
class PullMessageResponseHeader : public CommandHeader {
 public:
  PullMessageResponseHeader() : suggestWhichBrokerId(0), nextBeginOffset(0), minOffset(0), maxOffset(0) {}
  virtual ~PullMessageResponseHeader() {}
  static CommandHeader* Decode(Json::Value& ext);
  virtual void SetDeclaredFieldOfCommandHeader(std::map<std::string, std::string>& requestMap);

 public:
  int64 suggestWhichBrokerId;
  int64 nextBeginOffset;
  int64 minOffset;
  int64 maxOffset;
};

//<!************************************************************************
class GetConsumerListByGroupResponseHeader : public CommandHeader {
 public:
  GetConsumerListByGroupResponseHeader() {}
  virtual ~GetConsumerListByGroupResponseHeader() {}
  virtual void Encode(Json::Value& outData);
  virtual void SetDeclaredFieldOfCommandHeader(std::map<std::string, std::string>& requestMap);
};

//<!***************************************************************************
class GetMinOffsetRequestHeader : public CommandHeader {
 public:
  GetMinOffsetRequestHeader() : queueId(0){};
  virtual ~GetMinOffsetRequestHeader() {}
  virtual void Encode(Json::Value& outData);
  virtual void SetDeclaredFieldOfCommandHeader(std::map<std::string, std::string>& requestMap);

 public:
  std::string topic;
  int queueId;
};

//<!***************************************************************************
class GetMinOffsetResponseHeader : public CommandHeader {
 public:
  GetMinOffsetResponseHeader() : offset(0){};
  virtual ~GetMinOffsetResponseHeader() {}
  static CommandHeader* Decode(Json::Value& ext);
  virtual void SetDeclaredFieldOfCommandHeader(std::map<std::string, std::string>& requestMap);

 public:
  int64 offset;
};

//<!***************************************************************************
class GetMaxOffsetRequestHeader : public CommandHeader {
 public:
  GetMaxOffsetRequestHeader() : queueId(0){};
  virtual ~GetMaxOffsetRequestHeader() {}
  virtual void Encode(Json::Value& outData);
  virtual void SetDeclaredFieldOfCommandHeader(std::map<std::string, std::string>& requestMap);

 public:
  std::string topic;
  int queueId;
};

//<!***************************************************************************
class GetMaxOffsetResponseHeader : public CommandHeader {
 public:
  GetMaxOffsetResponseHeader() : offset(0){};
  virtual ~GetMaxOffsetResponseHeader() {}
  static CommandHeader* Decode(Json::Value& ext);
  virtual void SetDeclaredFieldOfCommandHeader(std::map<std::string, std::string>& requestMap);

 public:
  int64 offset;
};

//<!***************************************************************************
class SearchOffsetRequestHeader : public CommandHeader {
 public:
  SearchOffsetRequestHeader() : queueId(0), timestamp(0){};
  virtual ~SearchOffsetRequestHeader() {}
  virtual void Encode(Json::Value& outData);
  virtual void SetDeclaredFieldOfCommandHeader(std::map<std::string, std::string>& requestMap);

 public:
  std::string topic;
  int queueId;
  int64 timestamp;
};

//<!***************************************************************************
class SearchOffsetResponseHeader : public CommandHeader {
 public:
  SearchOffsetResponseHeader() : offset(0){};
  virtual ~SearchOffsetResponseHeader() {}
  static CommandHeader* Decode(Json::Value& ext);
  virtual void SetDeclaredFieldOfCommandHeader(std::map<std::string, std::string>& requestMap);

 public:
  int64 offset;
};

//<!***************************************************************************
class ViewMessageRequestHeader : public CommandHeader {
 public:
  ViewMessageRequestHeader() : offset(0){};
  virtual ~ViewMessageRequestHeader() {}
  virtual void Encode(Json::Value& outData);
  virtual void SetDeclaredFieldOfCommandHeader(std::map<std::string, std::string>& requestMap);

 public:
  int64 offset;
};

//<!***************************************************************************
class GetEarliestMsgStoretimeRequestHeader : public CommandHeader {
 public:
  GetEarliestMsgStoretimeRequestHeader() : queueId(0){};
  virtual ~GetEarliestMsgStoretimeRequestHeader() {}
  virtual void Encode(Json::Value& outData);
  virtual void SetDeclaredFieldOfCommandHeader(std::map<std::string, std::string>& requestMap);

 public:
  std::string topic;
  int queueId;
};

//<!***************************************************************************
class GetEarliestMsgStoretimeResponseHeader : public CommandHeader {
 public:
  GetEarliestMsgStoretimeResponseHeader() : timestamp(0){};
  virtual ~GetEarliestMsgStoretimeResponseHeader() {}
  static CommandHeader* Decode(Json::Value& ext);
  virtual void SetDeclaredFieldOfCommandHeader(std::map<std::string, std::string>& requestMap);

 public:
  int64 timestamp;
};

//<!***************************************************************************
class GetConsumerListByGroupRequestHeader : public CommandHeader {
 public:
  GetConsumerListByGroupRequestHeader(){};
  virtual ~GetConsumerListByGroupRequestHeader() {}
  virtual void Encode(Json::Value& outData);
  virtual void SetDeclaredFieldOfCommandHeader(std::map<std::string, std::string>& requestMap);

 public:
  std::string consumerGroup;
};

//<!************************************************************************
class QueryConsumerOffsetRequestHeader : public CommandHeader {
 public:
  QueryConsumerOffsetRequestHeader() : queueId(0){};
  virtual ~QueryConsumerOffsetRequestHeader() {}
  virtual void Encode(Json::Value& outData);
  virtual void SetDeclaredFieldOfCommandHeader(std::map<std::string, std::string>& requestMap);

 public:
  std::string consumerGroup;
  std::string topic;
  int queueId;
};

//<!************************************************************************
class QueryConsumerOffsetResponseHeader : public CommandHeader {
 public:
  QueryConsumerOffsetResponseHeader() : offset(0){};
  virtual ~QueryConsumerOffsetResponseHeader() {}
  static CommandHeader* Decode(Json::Value& ext);
  virtual void SetDeclaredFieldOfCommandHeader(std::map<std::string, std::string>& requestMap);

 public:
  int64 offset;
};

//<!************************************************************************
class UpdateConsumerOffsetRequestHeader : public CommandHeader {
 public:
  UpdateConsumerOffsetRequestHeader() : queueId(0), commitOffset(0){};
  virtual ~UpdateConsumerOffsetRequestHeader() {}
  virtual void Encode(Json::Value& outData);
  virtual void SetDeclaredFieldOfCommandHeader(std::map<std::string, std::string>& requestMap);

 public:
  std::string consumerGroup;
  std::string topic;
  int queueId;
  int64 commitOffset;
};

//<!***************************************************************************
class ConsumerSendMsgBackRequestHeader : public CommandHeader {
 public:
  ConsumerSendMsgBackRequestHeader() : delayLevel(0), offset(0){};
  virtual ~ConsumerSendMsgBackRequestHeader() {}
  virtual void Encode(Json::Value& outData);
  virtual void SetDeclaredFieldOfCommandHeader(std::map<std::string, std::string>& requestMap);

 public:
  std::string group;
  int delayLevel;
  int64 offset;
};

//<!***************************************************************************
class GetConsumerListByGroupResponseBody {
 public:
  GetConsumerListByGroupResponseBody(){};
  virtual ~GetConsumerListByGroupResponseBody() {}
  virtual void SetDeclaredFieldOfCommandHeader(std::map<std::string, std::string>& requestMap);

 public:
  static void Decode(const MemoryBlock* mem, std::vector<std::string>& cids);
};

class ResetOffsetRequestHeader : public CommandHeader {
 public:
  ResetOffsetRequestHeader() {}
  ~ResetOffsetRequestHeader() {}
  static CommandHeader* Decode(Json::Value& ext);
  void setTopic(const std::string& tmp);
  void setGroup(const std::string& tmp);
  void setTimeStamp(const int64& tmp);
  void setForceFlag(const bool& tmp);
  const std::string getTopic() const;
  const std::string getGroup() const;
  const int64 getTimeStamp() const;
  const bool getForceFlag() const;

 private:
  std::string topic;
  std::string group;
  int64 timestamp;
  bool isForce;
};

class GetConsumerRunningInfoRequestHeader : public CommandHeader {
 public:
  GetConsumerRunningInfoRequestHeader() {}
  virtual ~GetConsumerRunningInfoRequestHeader() {}
  virtual void Encode(Json::Value& outData);
  virtual void SetDeclaredFieldOfCommandHeader(std::map<std::string, std::string>& requestMap);
  static CommandHeader* Decode(Json::Value& ext);
  const std::string getConsumerGroup() const;
  void setConsumerGroup(const std::string& consumerGroup);
  const std::string getClientId() const;
  void setClientId(const std::string& clientId);
  const bool isJstackEnable() const;
  void setJstackEnable(const bool& jstackEnable);

 private:
  std::string consumerGroup;
  std::string clientId;
  bool jstackEnable;
};

class NotifyConsumerIdsChangedRequestHeader : public CommandHeader {
 public:
  NotifyConsumerIdsChangedRequestHeader() {}
  virtual ~NotifyConsumerIdsChangedRequestHeader() {}
  static CommandHeader* Decode(Json::Value& ext);
  void setGroup(const std::string& tmp);
  const std::string getGroup() const;

 private:
  std::string consumerGroup;
};

//<!***************************************************************************
}  // namespace rocketmq

#endif
