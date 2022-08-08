#pragma once

#include <vector>

#include "Message.h"
#include "ONSClient.h"
#include "ONSPullStatus.h"

ONS_NAMESPACE_BEGIN

class ONSCLIENT_API PullResultONS {
public:
  PullResultONS(ONSPullStatus status) : pull_status_(status), next_begin_offset_(0), min_offset_(0), max_offset_(0) {
  }

  PullResultONS(ONSPullStatus status, long long next_begin_offset, long long min_offset, long long max_offset)
      : pull_status_(status), next_begin_offset_(next_begin_offset), min_offset_(min_offset), max_offset_(max_offset) {
  }

  virtual ~PullResultONS() = default;

  ONSPullStatus pull_status_;
  long long next_begin_offset_;
  long long min_offset_;
  long long max_offset_;
  std::vector<Message> message_list_;
};

ONS_NAMESPACE_END