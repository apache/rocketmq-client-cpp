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
#ifndef ROCKETMQ_COMMON_FILTERAPI_HPP_
#define ROCKETMQ_COMMON_FILTERAPI_HPP_

#include <string>  // std::string

#include "MQException.h"
#include "protocol/heartbeat/SubscriptionData.hpp"
#include "UtilAll.h"

namespace rocketmq {

class FilterAPI {
 public:
  static SubscriptionData* buildSubscriptionData(const std::string& topic, const std::string& sub_string) {
    // delete in Rebalance
    std::unique_ptr<SubscriptionData> subscription_data(new SubscriptionData(topic, sub_string));

    if (sub_string.empty() || SUB_ALL == sub_string) {
      subscription_data->set_sub_string(SUB_ALL);
    } else {
      std::vector<std::string> tags;
      UtilAll::Split(tags, sub_string, "||");

      if (!tags.empty()) {
        for (auto tag : tags) {
          if (!tag.empty()) {
            UtilAll::Trim(tag);
            if (!tag.empty()) {
              subscription_data->code_set().push_back(UtilAll::hash_code(tag));
              subscription_data->tags_set().push_back(std::move(tag));
            }
          }
        }
      } else {
        THROW_MQEXCEPTION(MQClientException, "FilterAPI subString split error", -1);
      }
    }

    return subscription_data.release();
  }
};

}  // namespace rocketmq

#endif  // ROCKETMQ_COMMON_FILTERAPI_HPP_
