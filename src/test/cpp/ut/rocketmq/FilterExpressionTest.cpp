#include "FilterExpression.h"
#include "gtest/gtest.h"

ROCKETMQ_NAMESPACE_BEGIN

class FilterExpressionTest : public testing::Test {
public:
  FilterExpressionTest() : filter_expression_("TagA") {}

  void SetUp() override {}

  void TearDown() override {}

protected:
  FilterExpression filter_expression_;
};

TEST_F(FilterExpressionTest, testAccept) {
  MQMessageExt message;
  message.setTags("TagA");
  EXPECT_TRUE(filter_expression_.accept(message));

  MQMessageExt message2;
  message2.setTags("TagB");
  EXPECT_FALSE(filter_expression_.accept(message2));
}

ROCKETMQ_NAMESPACE_END