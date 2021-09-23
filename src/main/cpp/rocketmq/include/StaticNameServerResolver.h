#pragma once

#include <cstdint>
#include <vector>
#include <atomic>

#include "absl/strings/string_view.h"

#include "NameServerResolver.h"
#include "rocketmq/RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

class StaticNameServerResolver : public NameServerResolver {
public:
  explicit StaticNameServerResolver(absl::string_view name_server_list);

  void start() override {}

  void shutdown() override {}

  std::string current() override;

  std::string next() override;

  std::vector<std::string> resolve() override;

private:
  std::vector<std::string> name_server_list_;
  std::atomic<std::uint32_t> index_{0};
};

ROCKETMQ_NAMESPACE_END