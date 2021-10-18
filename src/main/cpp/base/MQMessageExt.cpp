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
#include "rocketmq/MQMessageExt.h"
#include "MessageImpl.h"

ROCKETMQ_NAMESPACE_BEGIN

MQMessageExt::MQMessageExt() : MQMessage() {
}

MQMessageExt::MQMessageExt(const MQMessageExt& other) : MQMessage(other) {
}

MQMessageExt& MQMessageExt::operator=(const MQMessageExt& other) {
  if (this == &other) {
    return *this;
  }

  *impl_ = *(other.impl_);
  return *this;
}

int32_t MQMessageExt::getQueueId() const {
  return impl_->system_attribute_.partition_id;
}

std::chrono::system_clock::time_point MQMessageExt::bornTimestamp() const {
  return absl::ToChronoTime(impl_->system_attribute_.born_timestamp);
}

int64_t MQMessageExt::getBornTimestamp() const {
  return absl::ToUnixMillis(impl_->system_attribute_.born_timestamp);
}

std::chrono::system_clock::time_point MQMessageExt::storeTimestamp() const {
  return absl::ToChronoTime(impl_->system_attribute_.store_timestamp);
}

int64_t MQMessageExt::getStoreTimestamp() const {
  return absl::ToUnixMillis(impl_->system_attribute_.store_timestamp);
}

std::string MQMessageExt::getStoreHost() const {
  return impl_->system_attribute_.store_host;
}

int64_t MQMessageExt::getQueueOffset() const {
  return impl_->system_attribute_.partition_offset;
}

int32_t MQMessageExt::getDeliveryAttempt() const {
  return impl_->system_attribute_.attempt_times;
}

const std::string& MQMessageExt::receiptHandle() const {
  return impl_->system_attribute_.receipt_handle;
}

bool MQMessageExt::operator==(const MQMessageExt& other) {
  return impl_->system_attribute_.message_id == other.impl_->system_attribute_.message_id;
}

ROCKETMQ_NAMESPACE_END