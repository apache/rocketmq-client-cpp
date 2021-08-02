#include "rocketmq/CredentialsProvider.h"
#include "MixAll.h"
#include "ghc/filesystem.hpp"
#include "google/protobuf/struct.pb.h"
#include "google/protobuf/util/json_util.h"
#include "gtest/gtest.h"
#include <fstream>
#include <iostream>
#include <cstdlib>

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

TEST_F(CredentialsProviderTest, testEnvironmentVariable) {
  const char* access_key = "abc";
  const char* access_secret = "def";

  setenv(EnvironmentVariablesCredentialsProvider::ENVIRONMENT_ACCESS_KEY, access_key, 1);
  setenv(EnvironmentVariablesCredentialsProvider::ENVIRONMENT_ACCESS_SECRET, access_secret, 1);

  EnvironmentVariablesCredentialsProvider provider;
  const Credentials& credentials = provider.getCredentials();
  EXPECT_STREQ(access_key, credentials.accessKey().c_str());
  EXPECT_STREQ(access_secret, credentials.accessSecret().c_str());
}

ROCKETMQ_NAMESPACE_END