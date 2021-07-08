#include "RequestIdGenerator.h"

#include "absl/base/internal/sysinfo.h"
#include "MixAll.h"
#include "UtilAll.h"
#include "spdlog/spdlog.h"
#include <arpa/inet.h>

ROCKETMQ_NAMESPACE_BEGIN

std::atomic_long RequestIdGenerator::sequence_(0);

std::string RequestIdGenerator::nextRequestId() {
  int32_t numeric[4];
  std::string request_id;
  const std::string& ip = UtilAll::getHostIPv4();
  struct in_addr addr = {};
  int ret = inet_aton(ip.data(), &addr);

  // ip is supposed to be legal and parsable.
  assert(ret == 1);
  numeric[0] = htonl(addr.s_addr);
  numeric[1] = absl::base_internal::GetCachedTID();
  *(reinterpret_cast<int64_t*>(numeric) + 1) = ++sequence_;
  return MixAll::hex(numeric, sizeof(numeric));
}

int64_t RequestIdGenerator::sequenceOf(const std::string& request_id) {
  if (request_id.empty()) {
    SPDLOG_WARN("Invalid empty request_id");
    return 0;
  }
  std::vector<uint8_t> bin;
  if (!MixAll::hexToBinary(request_id, bin)) {
    SPDLOG_WARN("Invalid request_id: {}", request_id);
    return 0;
  }
  // IP(int32_t) + PID(int32_t) + term_id(int64_t)
  assert(bin.size() == 4 + 4 + 8);
  auto ptr = reinterpret_cast<int64_t*>(bin.data());
  return *(ptr + 1);
}

ROCKETMQ_NAMESPACE_END