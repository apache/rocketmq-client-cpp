#pragma once
#ifdef ENABLE_TRACING
#include "opentelemetry/trace/span_context.h"
#include "rocketmq/RocketMQ.h"
#include <iostream>
#include <string>

ROCKETMQ_NAMESPACE_BEGIN

namespace trace = opentelemetry::trace;

class TracingUtility {
public:
  static TracingUtility& get();

  static const int kTraceDelimiterBytes = 3;
  // 0: version, 1: trace id, 2: span id, 3: trace flags
  constexpr static const int kHeaderElementLengths[4] = {2, 32, 16, 2};
  static const int kHeaderSize = kHeaderElementLengths[0] + kHeaderElementLengths[1] + kHeaderElementLengths[2] +
                                 kHeaderElementLengths[3] + kTraceDelimiterBytes;

  static const int kTraceStateMaxMembers = 32;
  static const int kVersionBytes = 2;
  static const int kTraceIdBytes = 32;
  static const int kSpanIdBytes = 16;
  static const int kTraceFlagBytes = 2;

  static const std::string INVALID_TRACE_ID;
  static const std::string INVALID_SPAN_ID;

  const std::string topic_ = "topic";
  const std::string consumer_group_ = "consumer_group";
  const std::string msg_id_ = "msg_id";
  const std::string tags_ = "tags";
  const std::string store_host_ = "store_host";
  const std::string success_ = "success";
  const std::string max_attempt_times_ = "retry_time";
  const std::string expired_ = "expired";

  static std::string injectSpanContextToTraceParent(const trace::SpanContext& span_context);

  static trace::SpanContext extractContextFromTraceParent(const std::string& trace_parent);

private:
  static void generateHexFromString(const std::string& string, int bytes, uint8_t* buf);

  static uint8_t hexToInt(char c);

  static trace::TraceId generateTraceIdFromString(const std::string& trace_id);

  static bool isValidHex(const std::string& str);

  static trace::SpanId generateSpanIdFromString(const std::string& span_id);

  static trace::TraceFlags generateTraceFlagsFromString(std::string trace_flags);
};

ROCKETMQ_NAMESPACE_END
#endif