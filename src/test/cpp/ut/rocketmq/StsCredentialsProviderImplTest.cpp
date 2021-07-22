#include "StsCredentialsProviderImpl.h"
#include "HttpClientMock.h"
#include "absl/memory/memory.h"
#include "rocketmq/RocketMQ.h"
#include "gtest/gtest.h"
#include <algorithm>
#include <memory>
#include <string>
#include "grpc/grpc.h"

ROCKETMQ_NAMESPACE_BEGIN

class StsCredentialsProviderImplTest : public testing::Test {
public:
  void SetUp() override {
    grpc_init();
    sts_credentials_provider = std::make_shared<StsCredentialsProviderImpl>("test");
    auto http_client_ = absl::make_unique<testing::NiceMock<HttpClientMock>>();
    auto http_get_action =
        [](HttpProtocol protocol, const std::string& host, std::uint16_t port, const std::string& path,
           const std::function<void(int, const absl::flat_hash_map<std::string, std::string>&, const std::string&)>&
               cb) {
          absl::flat_hash_map<std::string, std::string> header;
          std::string body = R"(
            {
                "AccessKeyId": "key",
                "AccessKeySecret": "secret",
                "SecurityToken": "token",
                "Expiration" : "2017-11-01T05:20:01Z",
                "LastUpdated" : "2017-10-31T23:20:01Z",
                "Code" : "Success"
            }
          )";
          cb(200, header, body);
        };

    EXPECT_CALL(*http_client_, get).Times(testing::AtLeast(1)).WillRepeatedly(testing::Invoke(http_get_action));
    sts_credentials_provider->setHttpClient(std::move(http_client_));
  }

  void TearDown() override {
    grpc_shutdown();
  }

protected:
  std::shared_ptr<StsCredentialsProviderImpl> sts_credentials_provider;
};

TEST_F(StsCredentialsProviderImplTest, testGetCredentials) {
  auto credentials = sts_credentials_provider->getCredentials();
  EXPECT_EQ(credentials.accessKey(), "key");
  EXPECT_EQ(credentials.accessSecret(), "secret");
  absl::Time time;
  std::string input = "2017-11-01 05:20:01";
  std::string format = "%Y-%m-%d %H:%H:%S";
  std::string error;
  EXPECT_TRUE(absl::ParseTime(format, input, &time, &error));
  EXPECT_EQ(credentials.expirationInstant(), absl::ToChronoTime(time));
}

ROCKETMQ_NAMESPACE_END