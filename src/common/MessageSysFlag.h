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
#ifndef __MESSAGE_SYS_FLAG_H__
#define __MESSAGE_SYS_FLAG_H__

namespace rocketmq {

class MessageSysFlag {
 public:
  static int getTransactionValue(int flag);
  static int resetTransactionValue(int flag, int type);

  static int clearCompressedFlag(int flag);

 public:
  static const int CompressedFlag;
  static const int MultiTagsFlag;

  static const int TransactionNotType;
  static const int TransactionPreparedType;
  static const int TransactionCommitType;
  static const int TransactionRollbackType;
};

}  // namespace rocketmq

#endif  // __MESSAGE_SYS_FLAG_H__
