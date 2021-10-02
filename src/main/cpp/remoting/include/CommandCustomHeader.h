#pragma once

#include "google/protobuf/struct.pb.h"
#include "google/protobuf/util/json_util.h"

#include "rocketmq/RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

class CommandCustomHeader {
public:
  virtual ~CommandCustomHeader() = default;

  virtual void encode(google::protobuf::Value& root) const = 0;
};

ROCKETMQ_NAMESPACE_END