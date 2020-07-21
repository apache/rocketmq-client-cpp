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
#include "RequestFutureTable.h"

namespace rocketmq {

std::map<std::string, std::shared_ptr<RequestResponseFuture>> RequestFutureTable::future_table_;
std::mutex RequestFutureTable::future_table_mutex_;

void RequestFutureTable::putRequestFuture(std::string correlationId, std::shared_ptr<RequestResponseFuture> future) {
  std::lock_guard<std::mutex> lock(future_table_mutex_);
  future_table_[correlationId] = future;
}

std::shared_ptr<RequestResponseFuture> RequestFutureTable::removeRequestFuture(std::string correlationId) {
  std::lock_guard<std::mutex> lock(future_table_mutex_);
  const auto& it = future_table_.find(correlationId);
  if (it != future_table_.end()) {
    auto requestFuture = it->second;
    future_table_.erase(it);
    return requestFuture;
  }
  return nullptr;
}

}  // namespace rocketmq
