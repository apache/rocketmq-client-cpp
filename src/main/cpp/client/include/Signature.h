#pragma once

#include "absl/container/flat_hash_map.h"

#include "ClientConfig.h"

ROCKETMQ_NAMESPACE_BEGIN

class Signature {
public:
  static void sign(ClientConfig* client, absl::flat_hash_map<std::string, std::string>& metadata);
};

ROCKETMQ_NAMESPACE_END