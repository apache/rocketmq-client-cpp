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

  virtual int32_t store_size() const = 0;
  virtual void set_store_size(int32_t storeSize) = 0;

  virtual int32_t body_crc() const = 0;
  virtual void set_body_crc(int32_t bodyCRC) = 0;

  virtual int32_t queue_id() const = 0;
  virtual void set_queue_id(int32_t queueId) = 0;

  virtual int64_t queue_offset() const = 0;
  virtual void set_queue_offset(int64_t queueOffset) = 0;

  virtual int64_t commit_log_offset() const = 0;
  virtual void set_commit_log_offset(int64_t physicOffset) = 0;

  virtual int32_t sys_flag() const = 0;
  virtual void set_sys_flag(int32_t sysFlag) = 0;

  virtual int64_t born_timestamp() const = 0;
  virtual void set_born_timestamp(int64_t bornTimestamp) = 0;

  virtual std::string born_host_string() const = 0;
  virtual const struct sockaddr* born_host() const = 0;
  virtual void set_born_host(const struct sockaddr* bornHost) = 0;

  virtual int64_t store_timestamp() const = 0;
  virtual void set_store_timestamp(int64_t storeTimestamp) = 0;

  virtual std::string store_host_string() const = 0;
  virtual const struct sockaddr* store_host() const = 0;
  virtual void set_store_host(const struct sockaddr* storeHost) = 0;

  virtual int32_t reconsume_times() const = 0;
  virtual void set_reconsume_times(int32_t reconsumeTimes) = 0;

  virtual int64_t prepared_transaction_offset() const = 0;
  virtual void set_prepared_transaction_offset(int64_t preparedTransactionOffset) = 0;

  virtual const std::string& msg_id() const = 0;
  virtual void set_msg_id(const std::string& msgId) = 0;
};

}  // namespace rocketmq

#endif  // ROCKETMQ_MESSAGEEXT_H_
