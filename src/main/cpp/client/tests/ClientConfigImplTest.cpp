#include "ClientConfigImpl.h"

#include "absl/hash/hash_testing.h"
#include "rocketmq/TransportType.h"
#include "gtest/gtest.h"

ROCKETMQ_NAMESPACE_BEGIN

TEST(ClientConfigImplTest, testHashable) {
  ClientConfigImpl config10("abc");
  config10.resourceNamespace("ns://test");
  config10.transportType(TransportType::Grpc);

  ClientConfigImpl config11("abc");
  config11.resourceNamespace("ns://test");
  config11.transportType(TransportType::Remoting);

  ClientConfigImpl config20("def");
  config20.resourceNamespace("ns://test-2");
  config20.transportType(TransportType::Grpc);

  ClientConfigImpl config21("def");
  config21.resourceNamespace("ns://test-2");
  config21.transportType(TransportType::Remoting);

  EXPECT_TRUE(absl::VerifyTypeImplementsAbslHashCorrectly({
      config10,
      config11,
      config20,
      config21,
  }));
}

ROCKETMQ_NAMESPACE_END