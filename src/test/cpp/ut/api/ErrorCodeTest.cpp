#include "rocketmq/ErrorCode.h"
#include "gtest/gtest.h"
#include <system_error>

ROCKETMQ_NAMESPACE_BEGIN

TEST(ErrorCodeTest, testErrorCode) {
  std::error_code ec = ErrorCode::BadRequest;
  if (ec) {
    std::cout << ec.message() << std::endl;
  }
}

ROCKETMQ_NAMESPACE_END