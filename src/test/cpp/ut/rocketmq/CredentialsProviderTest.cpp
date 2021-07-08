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
public:
  void SetUp() override {
    MixAll::homeDirectory(config_file);
    config_file.push_back(ghc::filesystem::path::preferred_separator);
    config_file.append(ConfigFileCredentialsProvider::credentialFile());
    ghc::filesystem::path config_file_path(config_file);
    if (!ghc::filesystem::exists(config_file_path)) {
      delete_config_file_ = true;
      writeConfigFile();
    }
  }

  void TearDown() override {
    ghc::filesystem::path config_file_path(config_file);
    if (delete_config_file_ && ghc::filesystem::exists(config_file_path)) {
      ghc::filesystem::remove(config_file_path);
    }
  }

private:
  std::string config_file;
  bool delete_config_file_{false};

  void writeConfigFile() {
    ghc::filesystem::path config_file_path(config_file);
    if (!ghc::filesystem::exists(config_file_path.parent_path())) {
      ghc::filesystem::create_directories(config_file_path.parent_path());
    }

    if (!ghc::filesystem::exists(config_file_path)) {
      std::ofstream stream;
      stream.open(config_file.c_str(), std::ios::binary | std::ios::out | std::ios::app | std::ios::ate);
      google::protobuf::Struct root;
      auto fields = root.mutable_fields();

      google::protobuf::Value access_key_value;
      access_key_value.set_string_value("abc");
      fields->insert({"AccessKey", access_key_value});

      google::protobuf::Value access_secret_value;
      access_secret_value.set_string_value("def");
      fields->insert({"AccessSecret", access_secret_value});

      std::string json;
      google::protobuf::util::MessageToJsonString(root, &json);
      stream.write(json.c_str(), json.length());
    }
  }
};

TEST_F(CredentialsProviderTest, testSetUp) {
  ConfigFileCredentialsProvider provider;
  Credentials credentials = provider.getCredentials();
  ASSERT_FALSE(credentials.accessKey().empty());
  ASSERT_FALSE(credentials.accessSecret().empty());
}

TEST_F(CredentialsProviderTest, testStaticCredentialsProvider) {
  std::string access_key("abc");
  std::string access_secret("def");
  StaticCredentialsProvider credentials_provider(access_key, access_secret);
  Credentials&& credentials = credentials_provider.getCredentials();
  ASSERT_EQ(credentials.accessKey(), access_key);
  ASSERT_EQ(credentials.accessSecret(), access_secret);
}

ROCKETMQ_NAMESPACE_END