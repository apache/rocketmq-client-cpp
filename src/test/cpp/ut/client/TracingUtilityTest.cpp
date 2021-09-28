#include "TracingUtility.h"
#include "gtest/gtest.h"
#include <iostream>

using namespace opentelemetry::trace;

ROCKETMQ_NAMESPACE_BEGIN

template <typename T>
static std::string hex(const T& id_item) {
  char buf[T::kSize * 2];
  id_item.ToLowerBase16(buf);
  return std::string(buf, sizeof(buf));
}

TEST(TracingUtilityTest, testInject) {
  constexpr uint8_t buf_span[] = {1, 2, 3, 4, 5, 6, 7, 8};
  constexpr uint8_t buf_trace[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
  SpanContext span_context{TraceId{buf_trace}, SpanId{buf_span}, TraceFlags{true}, true};
  EXPECT_EQ(TracingUtility::injectSpanContextToTraceParent(span_context),
            "00-0102030405060708090a0b0c0d0e0f10-0102030405060708-01");
}

TEST(TracingUtilityTest, testExtract) {
  SpanContext span_context =
      TracingUtility::extractContextFromTraceParent("00-0102030405060708090a0b0c0d0e0f10-0102030405060708-01");

  EXPECT_EQ(hex(span_context.trace_id()), "0102030405060708090a0b0c0d0e0f10");
  EXPECT_EQ(hex(span_context.span_id()), "0102030405060708");
  EXPECT_TRUE(span_context.IsSampled());
  EXPECT_TRUE(span_context.IsRemote());
}

ROCKETMQ_NAMESPACE_END