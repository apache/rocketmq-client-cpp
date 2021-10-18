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
#pragma once

#include <atomic>
#include <vector>

#include "Assignment.h"
#include "RpcClient.h"

ROCKETMQ_NAMESPACE_BEGIN

class TopicAssignment {
public:
  explicit TopicAssignment(std::vector<Assignment>&& assignments) : assignment_list_(std::move(assignments)) {
    std::sort(assignment_list_.begin(), assignment_list_.end());
  }

  explicit TopicAssignment(const QueryAssignmentResponse& response);

  ~TopicAssignment() = default;

  const std::vector<Assignment>& assignmentList() const {
    return assignment_list_;
  }

  bool operator==(const TopicAssignment& rhs) const {
    return assignment_list_ == rhs.assignment_list_;
  }

  bool operator!=(const TopicAssignment& rhs) const {
    return assignment_list_ != rhs.assignment_list_;
  }

  const std::string& debugString() const {
    return debug_string_;
  }

  static unsigned int getAndIncreaseQueryWhichBroker() {
    return ++query_which_broker_;
  }

private:
  /**
   * Once it is set, it will be immutable.
   */
  std::vector<Assignment> assignment_list_;

  std::string debug_string_;

  thread_local static uint32_t query_which_broker_;
};

using TopicAssignmentPtr = std::shared_ptr<TopicAssignment>;

ROCKETMQ_NAMESPACE_END