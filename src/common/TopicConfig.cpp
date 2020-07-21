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
#include "TopicConfig.h"

#include <sstream>

#include "PermName.h"

static const std::string SEPARATOR = " ";

namespace rocketmq {

const int TopicConfig::DEFAULT_READ_QUEUE_NUMS = 16;
const int TopicConfig::DEFAULT_WRITE_QUEUE_NUMS = 16;

TopicConfig::TopicConfig()
    : topic_name_(null),
      read_queue_nums_(DEFAULT_READ_QUEUE_NUMS),
      write_queue_nums_(DEFAULT_WRITE_QUEUE_NUMS),
      perm_(PermName::PERM_READ | PermName::PERM_WRITE),
      topic_filter_type_(SINGLE_TAG) {}

TopicConfig::TopicConfig(const std::string& topicName)
    : topic_name_(topicName),
      read_queue_nums_(DEFAULT_READ_QUEUE_NUMS),
      write_queue_nums_(DEFAULT_WRITE_QUEUE_NUMS),
      perm_(PermName::PERM_READ | PermName::PERM_WRITE),
      topic_filter_type_(SINGLE_TAG) {}

TopicConfig::TopicConfig(const std::string& topicName, int readQueueNums, int writeQueueNums, int perm)
    : topic_name_(topicName),
      read_queue_nums_(readQueueNums),
      write_queue_nums_(writeQueueNums),
      perm_(perm),
      topic_filter_type_(SINGLE_TAG) {}

TopicConfig::~TopicConfig() {}

std::string TopicConfig::encode() const {
  std::stringstream ss;

  ss << topic_name_ << SEPARATOR << read_queue_nums_ << SEPARATOR << write_queue_nums_ << SEPARATOR << perm_
     << SEPARATOR << topic_filter_type_;

  return ss.str();
}

bool TopicConfig::decode(const std::string& in) {
  std::stringstream ss(in);

  ss >> topic_name_;
  ss >> read_queue_nums_;
  ss >> write_queue_nums_;
  ss >> perm_;

  int type;
  ss >> type;
  topic_filter_type_ = (TopicFilterType)type;

  return true;
}

}  // namespace rocketmq
