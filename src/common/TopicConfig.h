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
#ifndef ROCKETMQ_COMMON_TOPICCONFIG_H_
#define ROCKETMQ_COMMON_TOPICCONFIG_H_

#include <string>

#include "TopicFilterType.h"
#include "UtilAll.h"

namespace rocketmq {

class TopicConfig {
 public:
  static const int DEFAULT_READ_QUEUE_NUMS;
  static const int DEFAULT_WRITE_QUEUE_NUMS;

 public:
  TopicConfig();
  TopicConfig(const std::string& topicName);
  TopicConfig(const std::string& topicName, int readQueueNums, int writeQueueNums, int perm);
  ~TopicConfig();

  std::string encode() const;
  bool decode(const std::string& in);

 public:
  inline const std::string& topic_name() const { return topic_name_; }
  inline void set_topic_name(const std::string& topicName) { topic_name_ = topicName; }

  inline int read_queue_nums() const { return read_queue_nums_; }
  inline void set_read_queue_nums(int readQueueNums) { read_queue_nums_ = readQueueNums; }

  inline int write_queue_nums() const { return write_queue_nums_; }
  inline void set_write_queue_nums(int writeQueueNums) { write_queue_nums_ = writeQueueNums; }

  inline int perm() const { return perm_; }
  inline void set_perm(int perm) { perm_ = perm; }

  inline TopicFilterType topic_filter_type() const { return topic_filter_type_; }
  inline void set_topic_filter_type(TopicFilterType topicFilterType) { topic_filter_type_ = topicFilterType; }

 private:
  std::string topic_name_;
  int read_queue_nums_;
  int write_queue_nums_;
  int perm_;
  TopicFilterType topic_filter_type_;
};

}  // namespace rocketmq

#endif  // ROCKETMQ_COMMON_TOPICCONFIG_H_
