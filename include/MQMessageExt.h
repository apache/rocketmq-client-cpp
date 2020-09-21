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
  static std::vector<MQMessageExt> from_list(std::vector<MessageExtPtr>& msg_list);

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
  int32_t store_size() const override;
  void set_store_size(int32_t storeSize) override;

  int32_t body_crc() const override;
  void set_body_crc(int32_t bodyCRC) override;

  int32_t queue_id() const override;
  void set_queue_id(int32_t queueId) override;

  int64_t queue_offset() const override;
  void set_queue_offset(int64_t queueOffset) override;

  int64_t commit_log_offset() const override;
  void set_commit_log_offset(int64_t physicOffset) override;

  int32_t sys_flag() const override;
  void set_sys_flag(int32_t sysFlag) override;

  int64_t born_timestamp() const override;
  void set_born_timestamp(int64_t bornTimestamp) override;

  std::string born_host_string() const override;
  const struct sockaddr* born_host() const override;
  void set_born_host(const struct sockaddr* bornHost) override;

  int64_t store_timestamp() const override;
  void set_store_timestamp(int64_t storeTimestamp) override;

  std::string store_host_string() const override;
  const struct sockaddr* store_host() const override;
  void set_store_host(const struct sockaddr* storeHost) override;

  int32_t reconsume_times() const override;
  void set_reconsume_times(int32_t reconsumeTimes) override;

  int64_t prepared_transaction_offset() const override;
  void set_prepared_transaction_offset(int64_t preparedTransactionOffset) override;

  const std::string& msg_id() const override;
  void set_msg_id(const std::string& msgId) override;
};

}  // namespace rocketmq

#endif  // ROCKETMQ_MQMESSAGEEXT_H_
