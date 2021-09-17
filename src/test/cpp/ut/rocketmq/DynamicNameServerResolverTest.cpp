#include "DynamicNameServerResolver.h"

#include <chrono>
#include <map>
#include <memory>

#include "absl/memory/memory.h"
#include "absl/strings/str_join.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include <string>

#include "HttpClientMock.h"

ROCKETMQ_NAMESPACE_BEGIN

class DynamicNameServerResolverTest : public testing::Test {
public:
  DynamicNameServerResolverTest()
      : resolver_(std::make_shared<DynamicNameServerResolver>(endpoint_, std::chrono::seconds(1))) {}

  void SetUp() override {
    auto http_client = absl::make_unique<testing::NiceMock<HttpClientMock>>();

    auto callback =
        [this](HttpProtocol, const std::string&, std::uint16_t, const std::string&,
               const std::function<void(int, const std::multimap<std::string, std::string>&, const std::string&)>& cb) {
          int code = 200;
          std::multimap<std::string, std::string> headers;
          cb(code, headers, name_server_list_);
        };

    ON_CALL(*http_client, get).WillByDefault(testing::Invoke(callback));

    resolver_->injectHttpClient(std::move(http_client));

    resolver_->start();
  }

  void TearDown() override { resolver_->shutdown(); }

protected:
  std::string endpoint_{"http://jmenv.tbsite.net:8080/rocketmq/nsaddr"};
  std::string name_server_list_{"10.0.0.0:9876;10.0.0.1:9876"};
  std::shared_ptr<DynamicNameServerResolver> resolver_;
};

TEST_F(DynamicNameServerResolverTest, testResolve) {
  auto name_server_list = resolver_->resolve();
  ASSERT_FALSE(name_server_list.empty());
  std::string resolved = absl::StrJoin(name_server_list, ";");
  ASSERT_EQ(name_server_list_, resolved);

  std::string first{"10.0.0.0:9876"};
  EXPECT_EQ(first, resolver_->current());

  std::string second{"10.0.0.1:9876"};
  EXPECT_EQ(second, resolver_->next());
}

ROCKETMQ_NAMESPACE_END