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

#ifndef __ROCKETMQ_TRACE_CONTEXT_H__
#define __ROCKETMQ_TRACE_CONTEXT_H__

#include <string>
#include <vector>
#include "TraceBean.h"
#include "TraceConstant.h"

namespace rocketmq {
class TraceContext {
 public:
  TraceContext();

  TraceContext(const std::string& mGroupName);

  virtual ~TraceContext();

  TraceMessageType getMsgType() const;

  void setMsgType(TraceMessageType msgType);

  TraceType getTraceType() const;

  void setTraceType(TraceType traceType);

  long long int getTimeStamp() const;

  void setTimeStamp(long long int timeStamp);

  const std::string& getRegionId() const;

  void setRegionId(const std::string& regionId);

  const std::string& getGroupName() const;

  void setGroupName(const std::string& groupName);

  int getCostTime() const;

  void setCostTime(int costTime);

  bool getStatus() const;

  void setStatus(bool isSuccess);

  const std::string& getRequestId() const;

  void setRequestId(const std::string& requestId);

  int getTraceBeanIndex() const;

  void setTraceBeanIndex(int traceBeanIndex);

  const std::vector<TraceBean>& getTraceBeans() const;

  void setTraceBean(const TraceBean& traceBean);

 private:
  TraceMessageType m_msgType;
  TraceType m_traceType;
  long long m_timeStamp;
  std::string m_regionId;
  std::string m_groupName;
  int m_costTime;
  bool m_status;
  std::string m_requestId;
  int m_traceBeanIndex;
  std::vector<TraceBean> m_traceBeans;
};
}  // namespace rocketmq
#endif