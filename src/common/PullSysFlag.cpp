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
#include "PullSysFlag.h"

static const int FLAG_COMMIT_OFFSET = 0x1 << 0;
static const int FLAG_SUSPEND = 0x1 << 1;
static const int FLAG_SUBSCRIPTION = 0x1 << 2;
static const int FLAG_CLASS_FILTER = 0x1 << 3;
static const int FLAG_LITE_PULL_MESSAGE = 0x1 << 4;

namespace rocketmq {

int PullSysFlag::buildSysFlag(bool commitOffset, bool suspend, bool subscription, bool classFilter, bool litePull) {
  int flag = 0;

  if (commitOffset) {
    flag |= FLAG_COMMIT_OFFSET;
  }

  if (suspend) {
    flag |= FLAG_SUSPEND;
  }

  if (subscription) {
    flag |= FLAG_SUBSCRIPTION;
  }

  if (classFilter) {
    flag |= FLAG_CLASS_FILTER;
  }

  if (litePull) {
    flag |= FLAG_LITE_PULL_MESSAGE;
  }

  return flag;
}

int PullSysFlag::clearCommitOffsetFlag(int sysFlag) {
  return sysFlag & (~FLAG_COMMIT_OFFSET);
}

bool PullSysFlag::hasCommitOffsetFlag(int sysFlag) {
  return (sysFlag & FLAG_COMMIT_OFFSET) == FLAG_COMMIT_OFFSET;
}

bool PullSysFlag::hasSuspendFlag(int sysFlag) {
  return (sysFlag & FLAG_SUSPEND) == FLAG_SUSPEND;
}

bool PullSysFlag::hasSubscriptionFlag(int sysFlag) {
  return (sysFlag & FLAG_SUBSCRIPTION) == FLAG_SUBSCRIPTION;
}

bool PullSysFlag::hasClassFilterFlag(int sysFlag) {
  return (sysFlag & FLAG_CLASS_FILTER) == FLAG_CLASS_FILTER;
}

bool PullSysFlag::hasLitePullFlag(int sysFlag) {
  return (sysFlag & FLAG_LITE_PULL_MESSAGE) == FLAG_LITE_PULL_MESSAGE;
}

}  // namespace rocketmq
