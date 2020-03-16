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

#include "TraceContext.h"
#include <string>
#include <vector>

#include "StringIdMaker.h"
#include "UtilAll.h"

namespace rocketmq {
TraceContext::TraceContext() : m_timeStamp(UtilAll::currentTimeMillis()) {
  m_requestId = StringIdMaker::getInstance().createUniqID();
}

TraceContext::TraceContext(const std::string& mGroupName) : m_groupName(mGroupName) {}

TraceContext::~TraceContext() {}

TraceMessageType TraceContext::getMsgType() const {
  return m_msgType;
}

void TraceContext::setMsgType(TraceMessageType msgType) {
  m_msgType = msgType;
}

TraceType TraceContext::getTraceType() const {
  return m_traceType;
}

void TraceContext::setTraceType(TraceType traceType) {
  m_traceType = traceType;
}

long long int TraceContext::getTimeStamp() const {
  return m_timeStamp;
}

void TraceContext::setTimeStamp(long long int timeStamp) {
  m_timeStamp = timeStamp;
}

const string& TraceContext::getRegionId() const {
  return m_regionId;
}

void TraceContext::setRegionId(const string& regionId) {
  m_regionId = regionId;
}

const string& TraceContext::getGroupName() const {
  return m_groupName;
}

void TraceContext::setGroupName(const string& groupName) {
  m_groupName = groupName;
}

int TraceContext::getCostTime() const {
  return m_costTime;
}

void TraceContext::setCostTime(int costTime) {
  m_costTime = costTime;
}

bool TraceContext::getStatus() const {
  return m_status;
}

void TraceContext::setStatus(bool isSuccess) {
  m_status = isSuccess;
}

const string& TraceContext::getRequestId() const {
  return m_requestId;
}

void TraceContext::setRequestId(const string& requestId) {
  m_requestId = requestId;
}

int TraceContext::getTraceBeanIndex() const {
  return m_traceBeanIndex;
}

void TraceContext::setTraceBeanIndex(int traceBeanIndex) {
  m_traceBeanIndex = traceBeanIndex;
}

const vector<TraceBean>& TraceContext::getTraceBeans() const {
  return m_traceBeans;
}

void TraceContext::setTraceBean(const TraceBean& traceBean) {
  m_traceBeans.push_back(traceBean);
}
}  // namespace rocketmq