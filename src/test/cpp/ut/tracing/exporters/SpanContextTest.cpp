#include "opencensus/trace/propagation/trace_context.h"
#include "gtest/gtest.h"

TEST(SpanContextTest, testSpanContext) {
  std::string empty_context;
  opencensus::trace::SpanContext span_context;
  EXPECT_NO_THROW(span_context = opencensus::trace::propagation::FromTraceParentHeader(empty_context));
  EXPECT_FALSE(span_context.IsValid());
}

