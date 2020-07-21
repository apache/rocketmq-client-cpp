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
#ifndef ROCKETMQ_PROTOCOL_RESPOONSECODE_H_
#define ROCKETMQ_PROTOCOL_RESPOONSECODE_H_

namespace rocketmq {

enum MQResponseCode {
  // rcv success response from broker
  SUCCESS = 0,
  // rcv exception from broker
  SYSTEM_ERROR = 1,
  // rcv symtem busy from broker
  SYSTEM_BUSY = 2,
  // broker don't support the request code
  REQUEST_CODE_NOT_SUPPORTED = 3,
  // broker flush disk timeout error
  FLUSH_DISK_TIMEOUT = 10,
  // broker sync double write, slave broker not available
  SLAVE_NOT_AVAILABLE = 11,
  // broker sync double write, slave broker flush msg timeout
  FLUSH_SLAVE_TIMEOUT = 12,
  // broker rcv illegal mesage
  MESSAGE_ILLEGAL = 13,
  // service not available due to broker or namesrv in shutdown status
  SERVICE_NOT_AVAILABLE = 14,
  // this version is not supported on broker or namesrv
  VERSION_NOT_SUPPORTED = 15,
  // broker or Namesrv has no permission to do this operation
  NO_PERMISSION = 16,
  // topic is not exist on broker
  TOPIC_NOT_EXIST = 17,
  // broker already created this topic
  TOPIC_EXIST_ALREADY = 18,
  // pulled msg was not found
  PULL_NOT_FOUND = 19,
  // retry later
  PULL_RETRY_IMMEDIATELY = 20,
  // pull msg with wrong offset
  PULL_OFFSET_MOVED = 21,
  // could not find the query msg
  QUERY_NOT_FOUND = 22,

  SUBSCRIPTION_PARSE_FAILED = 23,
  SUBSCRIPTION_NOT_EXIST = 24,
  SUBSCRIPTION_NOT_LATEST = 25,
  SUBSCRIPTION_GROUP_NOT_EXIST = 26,

  TRANSACTION_SHOULD_COMMIT = 200,
  TRANSACTION_SHOULD_ROLLBACK = 201,
  TRANSACTION_STATE_UNKNOW = 202,
  TRANSACTION_STATE_GROUP_WRONG = 203,

  CONSUMER_NOT_ONLINE = 206,
  CONSUME_MSG_TIMEOUT = 207
};

}  // namespace rocketmq

#endif  // ROCKETMQ_PROTOCOL_RESPOONSECODE_H_
