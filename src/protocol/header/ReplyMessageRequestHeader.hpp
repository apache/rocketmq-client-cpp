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
#ifndef ROCKETMQ_PROTOCOL_HEADER_REPLY_MESSAGE_REQUEST_HEADER_HPP_
#define ROCKETMQ_PROTOCOL_HEADER_REPLY_MESSAGE_REQUEST_HEADER_HPP_

#include <vector>

#include "CommandCustomHeader.h"
#include "UtilAll.h"

namespace rocketmq {

class ReplyMessageRequestHeader : public CommandCustomHeader {
 public:
  static ReplyMessageRequestHeader* Decode(std::map<std::string, std::string>& extFields) {
    std::unique_ptr<ReplyMessageRequestHeader> header(new ReplyMessageRequestHeader());

    header->producer_group_ = extFields.at("producerGroup");
    header->topic_ = extFields.at("topic");
    header->default_topic_ = extFields.at("defaultTopic");
    header->default_topic_queue_nums_ = std::stoi(extFields.at("defaultTopicQueueNums"));
    header->queue_id_ = std::stoi(extFields.at("queueId"));
    header->sys_flag_ = std::stoi(extFields.at("sysFlag"));
    header->born_timestamp_ = std::stoll(extFields.at("bornTimestamp"));
    header->flag_ = std::stoi(extFields.at("flag"));

    auto it = extFields.find("properties");
    if (it != extFields.end()) {
      header->properties_ = it->second;
    }

    it = extFields.find("reconsumeTimes");
    if (it != extFields.end()) {
      header->reconsume_times_ = std::stoi(it->second);
    } else {
      header->reconsume_times_ = 0;
    }

    it = extFields.find("unitMode");
    if (it != extFields.end()) {
      header->unit_mode_ = UtilAll::stob(it->second);
    } else {
      header->unit_mode_ = false;
    }

    header->born_host_ = extFields.at("bornHost");
    header->store_host_ = extFields.at("storeHost");
    header->store_timestamp_ = std::stoll(extFields.at("storeTimestamp"));

    return header.release();
  }

 public:
  inline const std::string& producer_group() const { return this->producer_group_; }
  inline void set_producer_group(const std::string& producerGroup) { this->producer_group_ = producerGroup; }

  inline const std::string& topic() const { return this->topic_; }
  inline void set_topic(const std::string& topic) { this->topic_ = topic; }

  inline const std::string& default_topic() const { return this->default_topic_; }
  inline void set_default_topic(const std::string& defaultTopic) { this->default_topic_ = defaultTopic; }

  inline int32_t default_topic_queue_nums() const { return this->default_topic_queue_nums_; }
  inline void set_default_topic_queue_nums(int32_t defaultTopicQueueNums) {
    this->default_topic_queue_nums_ = defaultTopicQueueNums;
  }

  inline int32_t queue_id() const { return this->queue_id_; }
  inline void set_queue_id(int32_t queueId) { this->queue_id_ = queueId; }

  inline int32_t sys_flag() const { return this->sys_flag_; }
  inline void set_sys_flag(int32_t sysFlag) { this->sys_flag_ = sysFlag; }

  inline int64_t born_timestamp() const { return this->born_timestamp_; }
  inline void set_born_timestamp(int64_t bornTimestamp) { this->born_timestamp_ = bornTimestamp; }

  inline int32_t flag() const { return this->flag_; }
  inline void set_flag(int32_t flag) { this->flag_ = flag; }

  inline const std::string& properties() const { return this->properties_; }
  inline void set_properties(const std::string& properties) { this->properties_ = properties; }

  inline int32_t reconsume_times() const { return this->reconsume_times_; }
  inline void set_reconsume_times(int32_t reconsumeTimes) { this->reconsume_times_ = reconsumeTimes; }

  inline bool unit_mode() const { return this->unit_mode_; }
  inline void set_unit_mode(bool unitMode) { this->unit_mode_ = unitMode; }

  inline const std::string& born_host() const { return this->born_host_; }
  inline void set_born_host(const std::string& bornHost) { this->born_host_ = bornHost; }

  inline const std::string& store_host() const { return this->store_host_; }
  inline void set_store_host(const std::string& storeHost) { this->store_host_ = storeHost; }

  inline int64_t store_timestamp() const { return this->store_timestamp_; }
  inline void set_store_timestamp(int64_t storeTimestamp) { this->store_timestamp_ = storeTimestamp; }

 private:
  std::string producer_group_;
  std::string topic_;
  std::string default_topic_;
  int32_t default_topic_queue_nums_;
  int32_t queue_id_;
  int32_t sys_flag_;
  int64_t born_timestamp_;
  int32_t flag_;
  std::string properties_;   // nullable
  int32_t reconsume_times_;  // nullable
  bool unit_mode_;           // nullable

  std::string born_host_;
  std::string store_host_;
  int64_t store_timestamp_;
};

}  // namespace rocketmq

#endif  // ROCKETMQ_PROTOCOL_HEADER_REPLY_MESSAGE_REQUEST_HEADER_HPP_
