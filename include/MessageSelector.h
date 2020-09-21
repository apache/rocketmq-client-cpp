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
#ifndef ROCKETMQ_MESSAGESELECTOR_H_
#define ROCKETMQ_MESSAGESELECTOR_H_

#include <algorithm>  // std::move

#include "ExpressionType.h"

namespace rocketmq {

class ROCKETMQCLIENT_API MessageSelector {
 private:
  MessageSelector(const std::string& type, const std::string& expression) : type_(type), expression_(expression) {}

 public:
  virtual ~MessageSelector() = default;

  MessageSelector(const MessageSelector& other) : type_(other.type_), expression_(other.expression_) {}
  MessageSelector(MessageSelector&& other) : type_(std::move(other.type_)), expression_(std::move(other.expression_)) {}

  MessageSelector& operator=(const MessageSelector& other) {
    if (this != &other) {
      type_ = other.type_;
      expression_ = other.expression_;
    }
    return *this;
  }

  static inline MessageSelector bySql(const std::string& sql) { return MessageSelector(ExpressionType::SQL92, sql); }
  static inline MessageSelector byTag(const std::string& tag) { return MessageSelector(ExpressionType::TAG, tag); }

 public:
  const std::string& type() const { return type_; }
  const std::string& expression() const { return expression_; }

 private:
  std::string type_;
  std::string expression_;
};

}  // namespace rocketmq

#endif  // ROCKETMQ_MESSAGESELECTOR_H_
