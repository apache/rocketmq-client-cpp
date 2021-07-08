#include "TlsHelper.h"
#include <gtest/gtest.h>
#include <string>

ROCKETMQ_NAMESPACE_BEGIN

TEST(TlsHelperTest, testSign) {
  const char* data = "some random data for test purpose only";
  const char* access_secret = "arbitrary-access-key";
  const std::string& signature = TlsHelper::sign(access_secret, data);
  const char* expect = "567868dc8e81f1e8095f88958edff1e07db4290e";
  EXPECT_STRCASEEQ(expect, signature.c_str());
}

ROCKETMQ_NAMESPACE_END