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
#ifndef ROCKETMQ_MESSAGEEXT_H_
#define ROCKETMQ_MESSAGEEXT_H_

#ifdef WIN32
// clang-format off
#include <Winsock2.h>
#include <Windows.h>
// clang-format on
#else
#include <sys/socket.h>
#endif

#include "Message.h"

namespace rocketmq {

class MessageExt;
typedef std::shared_ptr<MessageExt> MessageExtPtr;

/**
 * MessageExt - Message extend interface, which was generated on broker
 */
class ROCKETMQCLIENT_API MessageExt : virtual public Message  // base interface
{
 public:
  virtual ~MessageExt() = default;

  virtual int32_t getStoreSize() const = 0;
  virtual void setStoreSize(int32_t storeSize) = 0;

  virtual int32_t getBodyCRC() const = 0;
  virtual void setBodyCRC(int32_t bodyCRC) = 0;

  virtual int32_t getQueueId() const = 0;
  virtual void setQueueId(int32_t queueId) = 0;

  virtual int64_t getQueueOffset() const = 0;
  virtual void setQueueOffset(int64_t queueOffset) = 0;

  virtual int64_t getCommitLogOffset() const = 0;
  virtual void setCommitLogOffset(int64_t physicOffset) = 0;

  virtual int32_t getSysFlag() const = 0;
  virtual void setSysFlag(int32_t sysFlag) = 0;

  virtual int64_t getBornTimestamp() const = 0;
  virtual void setBornTimestamp(int64_t bornTimestamp) = 0;

  virtual std::string getBornHostString() const = 0;
  virtual const struct sockaddr* getBornHost() const = 0;
  virtual void setBornHost(const struct sockaddr* bornHost) = 0;

  virtual int64_t getStoreTimestamp() const = 0;
  virtual void setStoreTimestamp(int64_t storeTimestamp) = 0;

  virtual std::string getStoreHostString() const = 0;
  virtual const struct sockaddr* getStoreHost() const = 0;
  virtual void setStoreHost(const struct sockaddr* storeHost) = 0;

  virtual int32_t getReconsumeTimes() const = 0;
  virtual void setReconsumeTimes(int32_t reconsumeTimes) = 0;

  virtual int64_t getPreparedTransactionOffset() const = 0;
  virtual void setPreparedTransactionOffset(int64_t preparedTransactionOffset) = 0;

  virtual const std::string& getMsgId() const = 0;
  virtual void setMsgId(const std::string& msgId) = 0;
};

}  // namespace rocketmq

#endif  // ROCKETMQ_MESSAGEEXT_H_
