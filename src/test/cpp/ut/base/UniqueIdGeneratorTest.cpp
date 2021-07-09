#include "UniqueIdGenerator.h"
#include "absl/container/flat_hash_set.h"
#include "rocketmq/RocketMQ.h"
#include "spdlog/spdlog.h"
#include "gtest/gtest.h"

ROCKETMQ_NAMESPACE_BEGIN

TEST(UniqueIdGeneratorTest, testNext) {
  absl::flat_hash_set<std::string> id_set;
  uint32_t total = 500000;
  uint32_t count = 0;
  while (total--) {
    std::string id = UniqueIdGenerator::instance().next();
    if (id_set.contains(id)) {
      SPDLOG_WARN("Yuck, found an duplicated ID: {}", id);
    } else {
      id_set.insert(id);
    }
    ++count;
  }
  EXPECT_EQ(count, id_set.size());
}

ROCKETMQ_NAMESPACE_END