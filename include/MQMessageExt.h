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
#ifndef ROCKETMQ_MQMESSAGEEXT_H_
#define ROCKETMQ_MQMESSAGEEXT_H_

#include "MessageExt.h"
#include "MQMessage.h"

namespace rocketmq {

/**
 * MQMessageExt - wrapper for MessageExt
 */
class ROCKETMQCLIENT_API MQMessageExt : public MQMessage,          // base
                                        virtual public MessageExt  // interface
{
 public:
  MQMessageExt();
  MQMessageExt(int queueId,
               int64_t bornTimestamp,
               const struct sockaddr* bornHost,
               int64_t storeTimestamp,
               const struct sockaddr* storeHost,
               const std::string& msgId);

 public:
  // reference constructor
  MQMessageExt(MessageExtPtr impl) : MQMessage(impl) {}

  // copy constructor
  MQMessageExt(const MQMessageExt& other) : MQMessage(other) {}
  MQMessageExt(MQMessageExt&& other) : MQMessage(std::move(other)) {}

  // assign operator
  MQMessageExt& operator=(const MQMessageExt& other) {
    if (this != &other) {
      message_impl_ = other.message_impl_;
    }
    return *this;
  }

 public:
  virtual ~MQMessageExt();

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

  std::string getBornHostString() const override;
  const struct sockaddr* getBornHost() const override;
  void setBornHost(const struct sockaddr* bornHost) override;

  int64_t getStoreTimestamp() const override;
  void setStoreTimestamp(int64_t storeTimestamp) override;

  std::string getStoreHostString() const override;
  const struct sockaddr* getStoreHost() const override;
  void setStoreHost(const struct sockaddr* storeHost) override;

  int32_t getReconsumeTimes() const override;
  void setReconsumeTimes(int32_t reconsumeTimes) override;

  int64_t getPreparedTransactionOffset() const override;
  void setPreparedTransactionOffset(int64_t preparedTransactionOffset) override;

  const std::string& getMsgId() const override;
  void setMsgId(const std::string& msgId) override;
};

}  // namespace rocketmq

#endif  // ROCKETMQ_MQMESSAGEEXT_H_
