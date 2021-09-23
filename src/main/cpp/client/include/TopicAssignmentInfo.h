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

  const std::vector<Assignment>& assignmentList() const { return assignment_list_; }

  bool operator==(const TopicAssignment& rhs) const { return assignment_list_ == rhs.assignment_list_; }

  bool operator!=(const TopicAssignment& rhs) const { return assignment_list_ != rhs.assignment_list_; }

  const std::string& debugString() const { return debug_string_; }

  static unsigned int getAndIncreaseQueryWhichBroker() { return ++query_which_broker_; }

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