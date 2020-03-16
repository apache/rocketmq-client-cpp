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

#include "TraceUtil.h"
#include <sstream>
#include <string>
#include "TraceContant.h"

namespace rocketmq {
std::string TraceUtil::CovertTraceTypeToString(TraceType type) {
  switch (type) {
    case Pub:
      return TraceContant::TRACE_TYPE_PUB;
    case SubBefore:
      return TraceContant::TRACE_TYPE_BEFORE;
    case SubAfter:
      return TraceContant::TRACE_TYPE_AFTER;
    default:
      return TraceContant::TRACE_TYPE_PUB;
  }
}

TraceTransferBean TraceUtil::CovertTraceContextToTransferBean(TraceContext* ctx) {
  std::ostringstream ss;
  std::vector<TraceBean> beans = ctx->getTraceBeans();
  switch (ctx->getTraceType()) {
    case Pub: {
      std::vector<TraceBean>::iterator it = beans.begin();
      ss << TraceUtil::CovertTraceTypeToString(ctx->getTraceType()) << TraceContant::CONTENT_SPLITOR;
      ss << ctx->getTimeStamp() << TraceContant::CONTENT_SPLITOR;
      ss << ctx->getRegionId() << TraceContant::CONTENT_SPLITOR;
      ss << ctx->getGroupName() << TraceContant::CONTENT_SPLITOR;
      ss << it->getTopic() << TraceContant::CONTENT_SPLITOR;
      ss << it->getMsgId() << TraceContant::CONTENT_SPLITOR;
      ss << it->getTags() << TraceContant::CONTENT_SPLITOR;
      ss << it->getKeys() << TraceContant::CONTENT_SPLITOR;
      ss << it->getStoreHost() << TraceContant::CONTENT_SPLITOR;
      ss << it->getBodyLength() << TraceContant::CONTENT_SPLITOR;
      ss << ctx->getCostTime() << TraceContant::CONTENT_SPLITOR;
      ss << it->getMsgType() << TraceContant::CONTENT_SPLITOR;
      ss << it->getOffsetMsgId() << TraceContant::CONTENT_SPLITOR;
      ss << (ctx->getStatus() ? "true" : "false") << TraceContant::FIELD_SPLITOR;
    } break;

    case SubBefore: {
      std::vector<TraceBean>::iterator it = beans.begin();
      for (; it != beans.end(); ++it) {
        ss << TraceUtil::CovertTraceTypeToString(ctx->getTraceType()) << TraceContant::CONTENT_SPLITOR;
        ss << ctx->getTimeStamp() << TraceContant::CONTENT_SPLITOR;
        ss << ctx->getRegionId() << TraceContant::CONTENT_SPLITOR;
        ss << ctx->getGroupName() << TraceContant::CONTENT_SPLITOR;
        ss << ctx->getRequestId() << TraceContant::CONTENT_SPLITOR;
        ss << it->getMsgId() << TraceContant::CONTENT_SPLITOR;
        ss << it->getRetryTimes() << TraceContant::CONTENT_SPLITOR;
        // this is a bug caused by broker.
        std::string defaultKey = "dKey";
        if (!it->getKeys().empty()) {
          defaultKey = it->getKeys();
        }
        ss << defaultKey << TraceContant::FIELD_SPLITOR;
      }
    } break;

    case SubAfter: {
      std::vector<TraceBean>::iterator it = beans.begin();
      ss << TraceUtil::CovertTraceTypeToString(ctx->getTraceType()) << TraceContant::CONTENT_SPLITOR;
      ss << ctx->getRequestId() << TraceContant::CONTENT_SPLITOR;
      ss << it->getMsgId() << TraceContant::CONTENT_SPLITOR;
      ss << ctx->getCostTime() << TraceContant::CONTENT_SPLITOR;
      ss << (ctx->getStatus() ? "true" : "false") << TraceContant::CONTENT_SPLITOR;
      // this is a bug caused by broker.
      std::string defaultKey = "dKey";
      if (!it->getKeys().empty()) {
        defaultKey = it->getKeys();
      }
      ss << defaultKey << TraceContant::FIELD_SPLITOR;
    } break;

    default:
      break;
  }

  TraceTransferBean transferBean;
  transferBean.setTransData(ss.str());

  switch (ctx->getTraceType()) {
    case Pub:
    case SubAfter: {
      std::vector<TraceBean>::iterator it = beans.begin();
      transferBean.setTransKey(it->getMsgId());
      if (it->getKeys() != "") {
        transferBean.setTransKey(it->getKeys());
      }
    } break;
    case SubBefore: {
      std::vector<TraceBean>::iterator it = beans.begin();
      for (; it != beans.end(); ++it) {
        transferBean.setTransKey((*it).getMsgId());
        if ((*it).getKeys() != "") {
          transferBean.setTransKey((*it).getKeys());
        }
      }
    } break;
    default:
      break;
  }

  return transferBean;
}
}  // namespace rocketmq
