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
#include "MQMessageConst.h"

namespace rocketmq {

const std::string MQMessageConst::PROPERTY_KEYS = "KEYS";
const std::string MQMessageConst::PROPERTY_TAGS = "TAGS";
const std::string MQMessageConst::PROPERTY_WAIT_STORE_MSG_OK = "WAIT";
const std::string MQMessageConst::PROPERTY_DELAY_TIME_LEVEL = "DELAY";
const std::string MQMessageConst::PROPERTY_RETRY_TOPIC = "RETRY_TOPIC";
const std::string MQMessageConst::PROPERTY_REAL_TOPIC = "REAL_TOPIC";
const std::string MQMessageConst::PROPERTY_REAL_QUEUE_ID = "REAL_QID";
const std::string MQMessageConst::PROPERTY_TRANSACTION_PREPARED = "TRAN_MSG";
const std::string MQMessageConst::PROPERTY_PRODUCER_GROUP = "PGROUP";
const std::string MQMessageConst::PROPERTY_MIN_OFFSET = "MIN_OFFSET";
const std::string MQMessageConst::PROPERTY_MAX_OFFSET = "MAX_OFFSET";

const std::string MQMessageConst::PROPERTY_BUYER_ID = "BUYER_ID";
const std::string MQMessageConst::PROPERTY_ORIGIN_MESSAGE_ID = "ORIGIN_MESSAGE_ID";
const std::string MQMessageConst::PROPERTY_TRANSFER_FLAG = "TRANSFER_FLAG";
const std::string MQMessageConst::PROPERTY_CORRECTION_FLAG = "CORRECTION_FLAG";
const std::string MQMessageConst::PROPERTY_MQ2_FLAG = "MQ2_FLAG";
const std::string MQMessageConst::PROPERTY_RECONSUME_TIME = "RECONSUME_TIME";
const std::string MQMessageConst::PROPERTY_MSG_REGION = "MSG_REGION";
const std::string MQMessageConst::PROPERTY_TRACE_SWITCH = "TRACE_ON";
const std::string MQMessageConst::PROPERTY_UNIQ_CLIENT_MESSAGE_ID_KEYIDX = "UNIQ_KEY";
const std::string MQMessageConst::PROPERTY_MAX_RECONSUME_TIMES = "MAX_RECONSUME_TIMES";
const std::string MQMessageConst::PROPERTY_CONSUME_START_TIMESTAMP = "CONSUME_START_TIME";
const std::string MQMessageConst::PROPERTY_TRANSACTION_PREPARED_QUEUE_OFFSET = "TRAN_PREPARED_QUEUE_OFFSET";
const std::string MQMessageConst::PROPERTY_TRANSACTION_CHECK_TIMES = "TRANSACTION_CHECK_TIMES";
const std::string MQMessageConst::PROPERTY_CHECK_IMMUNITY_TIME_IN_SECONDS = "CHECK_IMMUNITY_TIME_IN_SECONDS";
const std::string MQMessageConst::PROPERTY_INSTANCE_ID = "INSTANCE_ID";
const std::string MQMessageConst::PROPERTY_CORRELATION_ID = "CORRELATION_ID";
const std::string MQMessageConst::PROPERTY_MESSAGE_REPLY_TO_CLIENT = "REPLY_TO_CLIENT";
const std::string MQMessageConst::PROPERTY_MESSAGE_TTL = "TTL";
const std::string MQMessageConst::PROPERTY_REPLY_MESSAGE_ARRIVE_TIME = "ARRIVE_TIME";
const std::string MQMessageConst::PROPERTY_PUSH_REPLY_TIME = "PUSH_REPLY_TIME";
const std::string MQMessageConst::PROPERTY_CLUSTER = "CLUSTER";
const std::string MQMessageConst::PROPERTY_MESSAGE_TYPE = "MSG_TYPE";

const std::string MQMessageConst::PROPERTY_ALREADY_COMPRESSED_FLAG = "__ALREADY_COMPRESSED__";

const std::string MQMessageConst::KEY_SEPARATOR = " ";

}  // namespace rocketmq
