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
#ifndef ROCKETMQ_QUERYRESULT_H_
#define ROCKETMQ_QUERYRESULT_H_

#include "MQMessageExt.h"

namespace rocketmq {

class ROCKETMQCLIENT_API QueryResult {
 public:
  QueryResult(uint64_t indexLastUpdateTimestamp, const std::vector<MQMessageExt>& messageList) {
    index_last_update_timestamp_ = indexLastUpdateTimestamp;
    message_list_ = messageList;
  }

  uint64_t index_last_update_timestamp() { return index_last_update_timestamp_; }

  std::vector<MQMessageExt>& message_list() { return message_list_; }

 private:
  uint64_t index_last_update_timestamp_;
  std::vector<MQMessageExt> message_list_;
};

}  // namespace rocketmq

#endif  // ROCKETMQ_QUERYRESULT_H_
