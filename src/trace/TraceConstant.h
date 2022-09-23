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

#ifndef __ROCKETMQ_TRACE_CONTANT_H_
#define __ROCKETMQ_TRACE_CONTANT_H_

#include <string>

namespace rocketmq {
class TraceConstant {
 public:
  static std::string GROUP_NAME;
  static std::string TRACE_TOPIC;
  static std::string DEFAULT_REDION;
  static char CONTENT_SPLITOR;
  static char FIELD_SPLITOR;
  static std::string TRACE_TYPE_PUB;
  static std::string TRACE_TYPE_BEFORE;
  static std::string TRACE_TYPE_AFTER;
};
enum TraceMessageType {
  TRACE_NORMAL_MSG = 0,
  TRACE_TRANS_HALF_MSG,
  TRACE_TRANS_COMMIT_MSG,
  TRACE_DELAY_MSG,
};
enum TraceType {
  Pub,        // for send message
  SubBefore,  // for consume message before
  SubAfter,   // for consum message after
};
}  // namespace rocketmq
#endif  //
