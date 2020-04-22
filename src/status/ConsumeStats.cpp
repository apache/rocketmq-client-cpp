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

#include "ConsumeStats.h"

namespace rocketmq {
ConsumeStats::ConsumeStats() {
  pullRT = 0.0;
  pullTPS = 0.0;
  consumeRT = 0.0;
  consumeOKTPS = 0.0;
  consumeFailedTPS = 0.0;
  consumeFailedMsgs = 0;
}
Json::Value ConsumeStats::toJson() const {
  Json::Value outJson;
  outJson["pullRT"] = pullRT;
  outJson["pullTPS"] = pullTPS;
  outJson["consumeRT"] = consumeRT;
  outJson["consumeOKTPS"] = consumeOKTPS;
  outJson["consumeFailedTPS"] = consumeFailedTPS;
  outJson["consumeFailedMsgs"] = consumeFailedMsgs;
  return outJson;
}
}  // namespace rocketmq
