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
#include "BrokerData.h"

#include <string>

#include "CommandCustomHeader.h"

ROCKETMQ_NAMESPACE_BEGIN

BrokerData BrokerData::decode(const google::protobuf::Struct& root) {
  BrokerData broker_data;
  const auto& fields = root.fields();

  assign(fields, "cluster", &broker_data.cluster_);
  assign(fields, "brokerName", &broker_data.broker_name_);

  if (fields.contains("brokerAddrs")) {
    auto items = fields.at("brokerAddrs").struct_value().fields();
    for (const auto& item : items) {
      auto k = std::stoll(item.first);
      broker_data.broker_addresses_.insert({k, item.second.string_value()});
    }
  }
  return broker_data;
}

ROCKETMQ_NAMESPACE_END