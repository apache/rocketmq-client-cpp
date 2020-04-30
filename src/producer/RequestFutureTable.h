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
#ifndef __REQUEST_FUTURE_TABLE__
#define __REQUEST_FUTURE_TABLE__

#include <map>
#include <memory>
#include <thread>

#include "RequestResponseFuture.h"

namespace rocketmq {

class RequestFutureTable {
 public:
  static void putRequestFuture(std::string correlationId, std::shared_ptr<RequestResponseFuture> future);
  static std::shared_ptr<RequestResponseFuture> removeRequestFuture(std::string correlationId);

 private:
  static std::map<std::string, std::shared_ptr<RequestResponseFuture>> s_futureTable;
  static std::mutex s_futureTableMutex;
};

}  // namespace rocketmq

#endif  // __REQUEST_FUTURE_TABLE__
