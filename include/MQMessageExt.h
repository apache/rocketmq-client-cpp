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
#ifndef __MQ_MESSAGE_EXT_H__
#define __MQ_MESSAGE_EXT_H__

#ifdef WIN32
// clang-format off
#include <Winsock2.h>
#include <Windows.h>
// clang-format on
#else
#include <sys/socket.h>
#endif

#include "MQMessage.h"
#include "TopicFilterType.h"

namespace rocketmq {

class MQMessageExt;

typedef MQMessageExt* MQMessageExtPtr;
typedef std::shared_ptr<MQMessageExt> MQMessageExtPtr2;

// message extend class, which was generated on broker
class ROCKETMQCLIENT_API MQMessageExt : public MQMessage {
 public:
  MQMessageExt();
  MQMessageExt(int queueId,
               int64_t bornTimestamp,
               sockaddr bornHost,
               int64_t storeTimestamp,
               sockaddr storeHost,
               std::string msgId);

  virtual ~MQMessageExt();

  static TopicFilterType parseTopicFilterType(int32_t sysFlag);

  int32_t getStoreSize() const;
  void setStoreSize(int32_t storeSize);

  int32_t getBodyCRC() const;
  void setBodyCRC(int32_t bodyCRC);

  int32_t getQueueId() const;
  void setQueueId(int32_t queueId);

  int64_t getQueueOffset() const;
  void setQueueOffset(int64_t queueOffset);

  int64_t getCommitLogOffset() const;
  void setCommitLogOffset(int64_t physicOffset);

  int32_t getSysFlag() const;
  void setSysFlag(int32_t sysFlag);

  int64_t getBornTimestamp() const;
  void setBornTimestamp(int64_t bornTimestamp);

  const sockaddr& getBornHost() const;
  std::string getBornHostString() const;
  void setBornHost(const sockaddr& bornHost);

  int64_t getStoreTimestamp() const;
  void setStoreTimestamp(int64_t storeTimestamp);

  const sockaddr& getStoreHost() const;
  std::string getStoreHostString() const;
  void setStoreHost(const sockaddr& storeHost);

  int32_t getReconsumeTimes() const;
  void setReconsumeTimes(int32_t reconsumeTimes);

  int64_t getPreparedTransactionOffset() const;
  void setPreparedTransactionOffset(int64_t preparedTransactionOffset);

  virtual const std::string& getMsgId() const;
  virtual void setMsgId(const std::string& msgId);

  std::string toString() const override {
    std::stringstream ss;
    ss << "MessageExt [queueId=" << m_queueId << ", storeSize=" << m_storeSize << ", queueOffset=" << m_queueOffset
       << ", sysFlag=" << m_sysFlag << ", bornTimestamp=" << m_bornTimestamp << ", bornHost=" << getBornHostString()
       << ", storeTimestamp=" << m_storeTimestamp << ", storeHost=" << getStoreHostString() << ", msgId=" << getMsgId()
       << ", commitLogOffset=" << m_commitLogOffset << ", bodyCRC=" << m_bodyCRC
       << ", reconsumeTimes=" << m_reconsumeTimes << ", preparedTransactionOffset=" << m_preparedTransactionOffset
       << ", toString()=" << MQMessage::toString() << "]";
    return ss.str();
  }

 private:
  int32_t m_storeSize;
  int32_t m_bodyCRC;
  int32_t m_queueId;
  int64_t m_queueOffset;
  int64_t m_commitLogOffset;
  int32_t m_sysFlag;
  int64_t m_bornTimestamp;
  sockaddr m_bornHost;
  int64_t m_storeTimestamp;
  sockaddr m_storeHost;
  int32_t m_reconsumeTimes;
  int64_t m_preparedTransactionOffset;
  std::string m_msgId;
};

class ROCKETMQCLIENT_API MQMessageClientExt : public MQMessageExt {
  const std::string& getOffsetMsgId() const;
  void setOffsetMsgId(const std::string& offsetMsgId);

  const std::string& getMsgId() const override;
  void setMsgId(const std::string& msgId) override;
};

}  // namespace rocketmq

#endif  // __MQ_MESSAGE_EXT_H__
