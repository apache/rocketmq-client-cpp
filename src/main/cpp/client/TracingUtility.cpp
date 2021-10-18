/*
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifdef ENABLE_TRACING
#include "TracingUtility.h"
#include <utility>

ROCKETMQ_NAMESPACE_BEGIN

TracingUtility& TracingUtility::get() {
  static TracingUtility tracing_utility;
  return tracing_utility;
}

const std::string TracingUtility::INVALID_TRACE_ID = "00000000000000000000000000000000";
const std::string TracingUtility::INVALID_SPAN_ID = "0000000000000000";

uint8_t TracingUtility::hexToInt(char c) {
  if (c >= '0' && c <= '9') {
    return (int)(c - '0');
  } else if (c >= 'a' && c <= 'f') {
    return (int)(c - 'a' + 10);
  } else if (c >= 'A' && c <= 'F') {
    return (int)(c - 'A' + 10);
  } else {
    return -1;
  }
}

void TracingUtility::generateHexFromString(const std::string& string, int bytes, uint8_t* buf) {
  const char* str_id = string.data();
  for (int i = 0; i < bytes; i++) {
    int tmp = hexToInt(str_id[i]);
    if (tmp < 0) {
      for (int j = 0; j < bytes / 2; j++) {
        buf[j] = 0;
      }
      return;
    }
    if (i % 2 == 0) {
      buf[i / 2] = tmp * 16;
    } else {
      buf[i / 2] += tmp;
    }
  }
}

trace::TraceId TracingUtility::generateTraceIdFromString(const std::string& trace_id) {
  int trace_id_len = kHeaderElementLengths[1];
  uint8_t buf[kTraceIdBytes / 2];
  uint8_t* b_ptr = buf;
  TracingUtility::generateHexFromString(trace_id, trace_id_len, b_ptr);
  return trace::TraceId(buf);
}

bool TracingUtility::isValidHex(const std::string& str) {
  for (char i : str) {
    if (!(i >= '0' && i <= '9') && !(i >= 'a' && i <= 'f')) {
      return false;
    }
  }
  return true;
}

trace::SpanId TracingUtility::generateSpanIdFromString(const std::string& span_id) {
  int span_id_len = kHeaderElementLengths[2];
  uint8_t buf[kSpanIdBytes / 2];
  uint8_t* b_ptr = buf;
  generateHexFromString(span_id, span_id_len, b_ptr);
  return trace::SpanId(buf);
}

trace::TraceFlags TracingUtility::generateTraceFlagsFromString(std::string trace_flags) {
  if (trace_flags.length() > 2) {
    return trace::TraceFlags(0); // check for invalid length of flags
  }
  int tmp1 = hexToInt(trace_flags[0]);
  int tmp2 = hexToInt(trace_flags[1]);
  if (tmp1 < 0 || tmp2 < 0) {
    return trace::TraceFlags(0); // check for invalid char
  }
  uint8_t buf = tmp1 * 16 + tmp2;
  return trace::TraceFlags(buf);
}

std::string TracingUtility::injectSpanContextToTraceParent(const trace::SpanContext& span_context) {
  char trace_id[32];
  trace::TraceId(span_context.trace_id()).ToLowerBase16(trace_id);
  char span_id[16];
  trace::SpanId(span_context.span_id()).ToLowerBase16(span_id);
  char trace_flags[2];
  trace::TraceFlags(span_context.trace_flags()).ToLowerBase16(trace_flags);
  // Note: This is only temporary replacement for appendable string
  std::string hex_string = "00-";
  for (char i : trace_id) {
    hex_string.push_back(i);
  }
  hex_string.push_back('-');
  for (char i : span_id) {
    hex_string.push_back(i);
  }
  hex_string.push_back('-');
  for (char trace_flag : trace_flags) {
    hex_string.push_back(trace_flag);
  }
  return hex_string;
}

// Assumed that all span context is remote.
trace::SpanContext TracingUtility::extractContextFromTraceParent(const std::string& trace_parent) {
  if (trace_parent.length() != kHeaderSize || trace_parent[kHeaderElementLengths[0]] != '-' ||
      trace_parent[kHeaderElementLengths[0] + kHeaderElementLengths[1] + 1] != '-' ||
      trace_parent[kHeaderElementLengths[0] + kHeaderElementLengths[1] + kHeaderElementLengths[2] + 2] != '-') {
    // Unresolvable trace_parent header. Returning INVALID span context.
    return trace::SpanContext(false, false);
  }
  std::string version = trace_parent.substr(0, kHeaderElementLengths[0]);
  std::string trace_id = trace_parent.substr(kHeaderElementLengths[0] + 1, kHeaderElementLengths[1]);
  std::string span_id =
      trace_parent.substr(kHeaderElementLengths[0] + kHeaderElementLengths[1] + 2, kHeaderElementLengths[2]);
  std::string trace_flags =
      trace_parent.substr(kHeaderElementLengths[0] + kHeaderElementLengths[1] + kHeaderElementLengths[2] + 3);

  if (version == "ff" || trace_id == INVALID_TRACE_ID || span_id == INVALID_SPAN_ID) {
    return trace::SpanContext(false, false);
  }

  // validate ids
  if (!TracingUtility::isValidHex(version) || !TracingUtility::isValidHex(trace_id) ||
      !TracingUtility::isValidHex(span_id) || !TracingUtility::isValidHex(trace_flags)) {
    return trace::SpanContext(false, false);
  }

  trace::TraceId trace_id_obj = TracingUtility::generateTraceIdFromString(trace_id);
  trace::SpanId span_id_obj = TracingUtility::generateSpanIdFromString(span_id);
  trace::TraceFlags trace_flags_obj = generateTraceFlagsFromString(trace_flags);
  return trace::SpanContext(trace_id_obj, span_id_obj, trace_flags_obj, true);
}

ROCKETMQ_NAMESPACE_END
#endif