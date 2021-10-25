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
#include "TracingUtility.h"
#include "MixAll.h"
#include "absl/strings/str_join.h"
#include "rocketmq/CredentialsProvider.h"
#include "spdlog/spdlog.h"

ROCKETMQ_NAMESPACE_BEGIN

void TracingUtility::addUniversalSpanAttributes(const MQMessage& message, ClientConfig& client_config,
                                                opencensus::trace::Span& span) {
  span.AddAttribute(MixAll::SPAN_ATTRIBUTE_KEY_MESSAGING_ID, message.getMsgId());
  span.AddAttribute(MixAll::SPAN_ATTRIBUTE_KEY_MESSAGING_PAYLOAD_SIZE_BYTES, message.getBody().length());
  span.AddAttribute(MixAll::SPAN_ATTRIBUTE_KEY_ROCKETMQ_TAG, message.getTags());
  const std::vector<std::string>& keys = message.getKeys();
  if (!keys.empty()) {
    span.AddAnnotation(MixAll::SPAN_ANNOTATION_MESSAGE_KEYS,
                       {{MixAll::SPAN_ANNOTATION_MESSAGE_KEYS,
                         absl::StrJoin(keys.begin(), keys.end(), MixAll::MESSAGE_KEY_SEPARATOR)}});
  }
  switch (message.messageType()) {
    case MessageType::FIFO:
      span.AddAttribute(MixAll::SPAN_ATTRIBUTE_KEY_ROCKETMQ_MESSAGE_TYPE,
                        MixAll::SPAN_ATTRIBUTE_VALUE_ROCKETMQ_FIFO_MESSAGE);
      break;
    case MessageType::DELAY:
      span.AddAttribute(MixAll::SPAN_ATTRIBUTE_KEY_ROCKETMQ_MESSAGE_TYPE,
                        MixAll::SPAN_ATTRIBUTE_VALUE_ROCKETMQ_DELAY_MESSAGE);
      break;
    case MessageType::TRANSACTION:
      span.AddAttribute(MixAll::SPAN_ATTRIBUTE_KEY_ROCKETMQ_MESSAGE_TYPE,
                        MixAll::SPAN_ATTRIBUTE_VALUE_ROCKETMQ_TRANSACTION_MESSAGE);
      break;
    default:
      span.AddAttribute(MixAll::SPAN_ATTRIBUTE_KEY_ROCKETMQ_MESSAGE_TYPE,
                        MixAll::SPAN_ATTRIBUTE_VALUE_ROCKETMQ_NORMAL_MESSAGE);
      break;
  }
  std::chrono::system_clock::time_point timestamp = message.deliveryTimestamp();
  auto duration = absl::FromChrono(timestamp.time_since_epoch());
  int64_t timestamp_millis = absl::ToInt64Milliseconds(duration);
  if (timestamp_millis > 0) {
    span.AddAttribute(MixAll::SPAN_ATTRIBUTE_KEY_ROCKETMQ_DELIVERY_TIMESTAMP, timestamp_millis);
  }
  span.AddAttribute(MixAll::SPAN_ATTRIBUTE_KEY_MESSAGING_SYSTEM,
                    MixAll::SPAN_ATTRIBUTE_VALUE_ROCKETMQ_MESSAGING_SYSTEM);
  span.AddAttribute(MixAll::SPAN_ATTRIBUTE_KEY_MESSAGING_DESTINATION, message.getTopic());
  span.AddAttribute(MixAll::SPAN_ATTRIBUTE_KEY_MESSAGING_DESTINATION_KIND,
                    MixAll::SPAN_ATTRIBUTE_VALUE_DESTINATION_KIND);
  span.AddAttribute(MixAll::SPAN_ATTRIBUTE_KEY_MESSAGING_PROTOCOL, MixAll::SPAN_ATTRIBUTE_VALUE_MESSAGING_PROTOCOL);
  span.AddAttribute(MixAll::SPAN_ATTRIBUTE_KEY_MESSAGING_PROTOCOL_VERSION,
                    MixAll::SPAN_ATTRIBUTE_VALUE_MESSAGING_PROTOCOL_VERSION);
  //   span.AddAttribute(MixAll::SPAN_ATTRIBUTE_KEY_MESSAGING_URL, "abc")
  span.AddAttribute(MixAll::SPAN_ATTRIBUTE_KEY_ROCKETMQ_NAMESPACE, client_config.resourceNamespace());
  span.AddAttribute(MixAll::SPAN_ATTRIBUTE_KEY_ROCKETMQ_CLIENT_ID, client_config.clientId());
  span.AddAttribute(MixAll::SPAN_ATTRIBUTE_KEY_ROCKETMQ_CLIENT_GROUP, client_config.getGroupName());

  span.AddAttribute(MixAll::SPAN_ATTRIBUTE_KEY_ROCKETMQ_ACCESS_KEY,
                    client_config.credentialsProvider()->getCredentials().accessKey());
  span.AddAttribute(MixAll::SPAN_ATTRIBUTE_KEY_ROCKETMQ_NAMESPACE, client_config.resourceNamespace());
}

ROCKETMQ_NAMESPACE_END