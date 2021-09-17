#include "StaticNameServerResolver.h"

#include "absl/strings/str_split.h"
#include <atomic>
#include <cstdint>

ROCKETMQ_NAMESPACE_BEGIN

StaticNameServerResolver::StaticNameServerResolver(absl::string_view name_server_list)
    : name_server_list_(absl::StrSplit(name_server_list, ';')) {}

std::string StaticNameServerResolver::current() {
  std::uint32_t index = index_.load(std::memory_order_relaxed) % name_server_list_.size();
  return name_server_list_[index];
}

std::string StaticNameServerResolver::next() {
  index_.fetch_add(1, std::memory_order_relaxed);
  return current();
}

std::vector<std::string> StaticNameServerResolver::resolve() { return name_server_list_; }

ROCKETMQ_NAMESPACE_END