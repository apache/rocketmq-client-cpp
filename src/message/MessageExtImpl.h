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
#ifndef ROCKETMQ_MESSAGE_MESSAGEEXT_H_
#define ROCKETMQ_MESSAGE_MESSAGEEXT_H_

#include "MessageExt.h"
#include "MessageImpl.h"
#include "TopicFilterType.h"

namespace rocketmq {

/**
 * MessageExtImpl -  MessageExt implement
 */
class MessageExtImpl : public MessageImpl,        // base
                       virtual public MessageExt  // interface
{
 public:
  MessageExtImpl();
  MessageExtImpl(int queueId,
                 int64_t bornTimestamp,
                 const struct sockaddr* bornHost,
                 int64_t storeTimestamp,
                 const struct sockaddr* storeHost,
                 const std::string& msgId);

  virtual ~MessageExtImpl();

  static TopicFilterType parseTopicFilterType(int32_t sysFlag);

 public:  // MessageExt
  int32_t getStoreSize() const override;
  void setStoreSize(int32_t storeSize) override;

  int32_t getBodyCRC() const override;
  void setBodyCRC(int32_t bodyCRC) override;

  int32_t getQueueId() const override;
  void setQueueId(int32_t queueId) override;

  int64_t getQueueOffset() const override;
  void setQueueOffset(int64_t queueOffset) override;

  int64_t getCommitLogOffset() const override;
  void setCommitLogOffset(int64_t physicOffset) override;

  int32_t getSysFlag() const override;
  void setSysFlag(int32_t sysFlag) override;

  int64_t getBornTimestamp() const override;
  void setBornTimestamp(int64_t bornTimestamp) override;

  const struct sockaddr* getBornHost() const override;
  std::string getBornHostString() const override;
  void setBornHost(const struct sockaddr* bornHost) override;

  int64_t getStoreTimestamp() const override;
  void setStoreTimestamp(int64_t storeTimestamp) override;

  const struct sockaddr* getStoreHost() const override;
  std::string getStoreHostString() const override;
  void setStoreHost(const struct sockaddr* storeHost) override;

  int32_t getReconsumeTimes() const override;
  void setReconsumeTimes(int32_t reconsumeTimes) override;

  int64_t getPreparedTransactionOffset() const override;
  void setPreparedTransactionOffset(int64_t preparedTransactionOffset) override;

  const std::string& getMsgId() const override;
  void setMsgId(const std::string& msgId) override;

  std::string toString() const override;

 private:
  int32_t store_size_;
  int32_t body_crc_;
  int32_t queue_id_;
  int64_t queue_offset_;
  int64_t commit_log_offset_;
  int32_t sys_flag_;
  int64_t born_timestamp_;
  struct sockaddr* born_host_;
  int64_t store_timestamp_;
  struct sockaddr* store_host_;
  int32_t reconsume_times_;
  int64_t prepared_transaction_offset_;
  std::string msg_id_;
};

class MessageClientExtImpl : public MessageExtImpl {
 public:  // MessageExt
  const std::string& getMsgId() const override;
  void setMsgId(const std::string& msgId) override;

 public:
  const std::string& getOffsetMsgId() const;
  void setOffsetMsgId(const std::string& offsetMsgId);
};

}  // namespace rocketmq

#endif  // ROCKETMQ_MESSAGE_MESSAGEEXT_H_
