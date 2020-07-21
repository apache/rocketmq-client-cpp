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
#ifndef ROCKETMQ_MQMESSAGECONST_H_
#define ROCKETMQ_MQMESSAGECONST_H_

#include <string>  // std::string

#include "RocketMQClient.h"

namespace rocketmq {

class ROCKETMQCLIENT_API MQMessageConst {
 public:
  static const std::string PROPERTY_KEYS;
  static const std::string PROPERTY_TAGS;
  static const std::string PROPERTY_WAIT_STORE_MSG_OK;
  static const std::string PROPERTY_DELAY_TIME_LEVEL;
  static const std::string PROPERTY_RETRY_TOPIC;
  static const std::string PROPERTY_REAL_TOPIC;
  static const std::string PROPERTY_REAL_QUEUE_ID;
  static const std::string PROPERTY_TRANSACTION_PREPARED;
  static const std::string PROPERTY_PRODUCER_GROUP;
  static const std::string PROPERTY_MIN_OFFSET;
  static const std::string PROPERTY_MAX_OFFSET;

  static const std::string PROPERTY_BUYER_ID;
  static const std::string PROPERTY_ORIGIN_MESSAGE_ID;
  static const std::string PROPERTY_TRANSFER_FLAG;
  static const std::string PROPERTY_CORRECTION_FLAG;
  static const std::string PROPERTY_MQ2_FLAG;
  static const std::string PROPERTY_RECONSUME_TIME;
  static const std::string PROPERTY_MSG_REGION;
  static const std::string PROPERTY_TRACE_SWITCH;
  static const std::string PROPERTY_UNIQ_CLIENT_MESSAGE_ID_KEYIDX;
  static const std::string PROPERTY_MAX_RECONSUME_TIMES;
  static const std::string PROPERTY_CONSUME_START_TIMESTAMP;
  static const std::string PROPERTY_TRANSACTION_PREPARED_QUEUE_OFFSET;
  static const std::string PROPERTY_TRANSACTION_CHECK_TIMES;
  static const std::string PROPERTY_CHECK_IMMUNITY_TIME_IN_SECONDS;
  static const std::string PROPERTY_INSTANCE_ID;
  static const std::string PROPERTY_CORRELATION_ID;
  static const std::string PROPERTY_MESSAGE_REPLY_TO_CLIENT;
  static const std::string PROPERTY_MESSAGE_TTL;
  static const std::string PROPERTY_REPLY_MESSAGE_ARRIVE_TIME;
  static const std::string PROPERTY_PUSH_REPLY_TIME;
  static const std::string PROPERTY_CLUSTER;
  static const std::string PROPERTY_MESSAGE_TYPE;

  // sdk internal use only
  static const std::string PROPERTY_ALREADY_COMPRESSED_FLAG;

  static const std::string KEY_SEPARATOR;
};

}  // namespace rocketmq

#endif  // ROCKETMQ_MQMESSAGECONST_H_
