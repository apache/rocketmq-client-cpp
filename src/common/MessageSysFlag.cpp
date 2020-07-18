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
#include "MessageSysFlag.h"

namespace rocketmq {

const int MessageSysFlag::CompressedFlag = 0x1 << 0;
const int MessageSysFlag::MultiTagsFlag = 0x1 << 1;

const int MessageSysFlag::TransactionNotType = 0x0;
const int MessageSysFlag::TransactionPreparedType = 0x1 << 2;
const int MessageSysFlag::TransactionCommitType = 0x2 << 2;
const int MessageSysFlag::TransactionRollbackType = 0x3 << 2;

const int MessageSysFlag::BronhostV6Flag = 0x1 << 4;
const int MessageSysFlag::StorehostV6Flag = 0x1 << 5;

int MessageSysFlag::getTransactionValue(int flag) {
  return flag & TransactionRollbackType;
}

int MessageSysFlag::resetTransactionValue(int flag, int type) {
  return (flag & (~TransactionRollbackType)) | type;
}

int MessageSysFlag::clearCompressedFlag(int flag) {
  return flag & (~CompressedFlag);
}

}  // namespace rocketmq
