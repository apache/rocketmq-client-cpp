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

#include "TraceConstant.h"
#include <string>

namespace rocketmq {
std::string TraceConstant::GROUP_NAME = "_INNER_TRACE_PRODUCER";
std::string TraceConstant::TRACE_TOPIC = "rmq_sys_TRACE_DATA_";
std::string TraceConstant::DEFAULT_REDION = "DEFAULT_REGION";
char TraceConstant::CONTENT_SPLITOR = 1;
char TraceConstant::FIELD_SPLITOR = 2;
std::string TraceConstant::TRACE_TYPE_PUB = "Pub";
std::string TraceConstant::TRACE_TYPE_BEFORE = "SubBefore";
std::string TraceConstant::TRACE_TYPE_AFTER = "SubAfter";
}  // namespace rocketmq
