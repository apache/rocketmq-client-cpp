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

#ifndef __ROCKETMQ_TRACE_BEAN_H__
#define __ROCKETMQ_TRACE_BEAN_H__

#include <string>
#include <vector>
#include "TraceConstant.h"

namespace rocketmq {
class TraceBean {
 public:
  TraceBean();
  virtual ~TraceBean();

  const std::string& getTopic() const;

  void setTopic(const std::string& topic);

  const std::string& getMsgId() const;

  void setMsgId(const std::string& msgId);

  const std::string& getOffsetMsgId() const;

  void setOffsetMsgId(const std::string& offsetMsgId);

  const std::string& getTags() const;

  void setTags(const std::string& tags);

  const std::string& getKeys() const;

  void setKeys(const std::string& keys);

  const std::string& getStoreHost() const;

  void setStoreHost(const std::string& storeHost);

  const std::string& getClientHost() const;

  void setClientHost(const std::string& clientHost);

  TraceMessageType getMsgType() const;

  void setMsgType(TraceMessageType msgType);

  long long int getStoreTime() const;

  void setStoreTime(long long int storeTime);

  int getRetryTimes() const;

  void setRetryTimes(int retryTimes);

  int getBodyLength() const;

  void setBodyLength(int bodyLength);

 private:
  std::string m_topic;
  std::string m_msgId;
  std::string m_offsetMsgId;
  std::string m_tags;
  std::string m_keys;
  std::string m_storeHost;
  std::string m_clientHost;
  TraceMessageType m_msgType;
  long long m_storeTime;
  int m_retryTimes;
  int m_bodyLength;
};
}  // namespace rocketmq
#endif