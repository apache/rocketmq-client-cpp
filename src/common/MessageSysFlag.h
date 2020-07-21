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
#ifndef ROCKETMQ_COMMON_MESSAGESYSFLAG_H_
#define ROCKETMQ_COMMON_MESSAGESYSFLAG_H_

namespace rocketmq {

class MessageSysFlag {
 public:
  static const int COMPRESSED_FLAG;
  static const int MULTI_TAGS_FLAG;

  static const int TRANSACTION_NOT_TYPE;
  static const int TRANSACTION_PREPARED_TYPE;
  static const int TRANSACTION_COMMIT_TYPE;
  static const int TRANSACTION_ROLLBACK_TYPE;

  static const int BORNHOST_V6_FLAG;
  static const int STOREHOST_V6_FLAG;

 public:
  static int getTransactionValue(int flag);
  static int resetTransactionValue(int flag, int type);

  static int clearCompressedFlag(int flag);
};

}  // namespace rocketmq

#endif  // ROCKETMQ_COMMON_MESSAGESYSFLAG_H_
