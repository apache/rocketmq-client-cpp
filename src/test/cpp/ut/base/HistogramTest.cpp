#include "Histogram.h"
#include <gtest/gtest.h>

ROCKETMQ_NAMESPACE_BEGIN

class HistogramTest : public testing::Test {
public:
  HistogramTest() : histogram_("Test", 3) {}

  void SetUp() override {
    histogram_.labels().emplace_back("Foo: ");
    histogram_.labels().emplace_back("Bar: ");
    histogram_.labels().emplace_back("Baz: ");
  }

protected:
  Histogram histogram_;
};

TEST_F(HistogramTest, testHistogram_no_count) {

  std::string result;
  histogram_.reportAndReset(result);

  std::string expect("Test:Foo: 0, Bar: 0, Baz: 0");
  EXPECT_STREQ(result.c_str(), expect.c_str());
}

TEST_F(HistogramTest, testHistogram) {
  histogram_.countIn(0);
  histogram_.countIn(-1);
  histogram_.countIn(4);
  histogram_.countIn(2);

  std::string result;
  histogram_.reportAndReset(result);

  std::string expect("Test:Foo: 1, Bar: 0, Baz: 2");
  EXPECT_STREQ(result.c_str(), expect.c_str());
}

ROCKETMQ_NAMESPACE_END