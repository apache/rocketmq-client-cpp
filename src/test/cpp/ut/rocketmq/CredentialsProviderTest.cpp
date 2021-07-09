#include "rocketmq/CredentialsProvider.h"
#include "MixAll.h"
#include "ghc/filesystem.hpp"
#include "google/protobuf/struct.pb.h"
#include "google/protobuf/util/json_util.h"
#include "gtest/gtest.h"
#include <fstream>
#include <iostream>

ROCKETMQ_NAMESPACE_BEGIN

class CredentialsProviderTest : public testing::Test {
};

TEST_F(CredentialsProviderTest, testStaticCredentialsProvider) {
  std::string access_key("abc");
  std::string access_secret("def");
  StaticCredentialsProvider credentials_provider(access_key, access_secret);
  Credentials&& credentials = credentials_provider.getCredentials();
  ASSERT_EQ(credentials.accessKey(), access_key);
  ASSERT_EQ(credentials.accessSecret(), access_secret);
}

ROCKETMQ_NAMESPACE_END