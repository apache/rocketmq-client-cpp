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
#ifndef __TRACEHELPER_H__
#define __TRACEHELPER_H__
#include <list>
#include <set>
#include <string>

namespace rocketmq {

enum MessageType {
  Normal_Msg,
  Trans_Msg_Half,
  Trans_msg_Commit,
  Delay_Msg,
};

enum TraceDispatcherType { PRODUCER, CONSUMER };

enum TraceType {
  Pub,
  SubBefore,
  SubAfter,
};

class TraceBean {
 public:
  static std::string LOCAL_ADDRESS;

 private:
  std::string m_topic;
  std::string m_msgId;
  std::string m_offsetMsgId;
  std::string m_tags;
  std::string m_keys;
  std::string m_storeHost;
  std::string m_clientHost;
  long m_storeTime;
  int m_retryTimes;
  int m_bodyLength;
  MessageType m_msgType;

 public:
  MessageType getMsgType() { return m_msgType; };

  void setMsgType(MessageType msgTypev) { m_msgType = msgTypev; };

  std::string getOffsetMsgId() { return m_offsetMsgId; };

  void setOffsetMsgId(std::string offsetMsgIdv) { m_offsetMsgId = offsetMsgIdv; };

  std::string getTopic() { return m_topic; };

  void setTopic(std::string topicv) { m_topic = topicv; };

  std::string getMsgId() { return m_msgId; };

  void setMsgId(std::string msgIdv) { m_msgId = msgIdv; };

  std::string getTags() { return m_tags; };

  void setTags(std::string tagsv) { m_tags = tagsv; };

  std::string getKeys() { return m_keys; };

  void setKeys(std::string keysv) { m_keys = keysv; }

  std::string getStoreHost() { return m_storeHost; };

  void setStoreHost(std::string storeHostv) { m_storeHost = storeHostv; };

  std::string getClientHost() { return m_clientHost; };

  void setClientHost(std::string clientHostv) { m_clientHost = clientHostv; };

  long getStoreTime() { return m_storeTime; };

  void setStoreTime(long storeTimev) { m_storeTime = storeTimev; };

  int getRetryTimes() { return m_retryTimes; };

  void setRetryTimes(int retryTimesv) { m_retryTimes = retryTimesv; };

  int getBodyLength() { return m_bodyLength; };

  void setBodyLength(int bodyLengthv) { m_bodyLength = bodyLengthv; };
};

class TraceConstants {
 public:
  static std::string TraceConstants_GROUP_NAME;
  static char CONTENT_SPLITOR;
  static char FIELD_SPLITOR;
  static std::string TRACE_INSTANCE_NAME;
  static std::string TRACE_TOPIC_PREFIX;
};

class TraceContext {
 private:
  TraceType m_traceType;
  long m_timeStamp;
  std::string m_regionId;
  std::string m_regionName;
  std::string m_groupName;
  int m_costTime;
  bool m_misSuccess;
  std::string m_requestId;
  int m_contextCode;
  std::list<TraceBean> m_traceBeans;

 public:
  int getContextCode() { return m_contextCode; };

  void setContextCode(int contextCodev) { m_contextCode = contextCodev; };

  std::list<TraceBean>& getTraceBeans() { return m_traceBeans; };

  void setTraceBeans(std::list<TraceBean> traceBeansv) { m_traceBeans = traceBeansv; };

  std::string getRegionId() { return m_regionId; };

  void setRegionId(std::string regionIdv) { m_regionId = regionIdv; };

  TraceType getTraceType() { return m_traceType; };

  void setTraceType(TraceType traceTypev) { m_traceType = traceTypev; };

  long getTimeStamp() { return m_timeStamp; };

  void setTimeStamp(long timeStampv) { m_timeStamp = timeStampv; };

  std::string getGroupName() { return m_groupName; };

  void setGroupName(std::string groupNamev) { m_groupName = groupNamev; };

  int getCostTime() { return m_costTime; };

  void setCostTime(int costTimev) { m_costTime = costTimev; };

  bool isSuccess() { return m_misSuccess; };

  void setSuccess(bool successv) { m_misSuccess = successv; };

  std::string getRequestId() { return m_requestId; };

  void setRequestId(std::string requestIdv) { m_requestId = requestIdv; };

  std::string getRegionName() { return m_regionName; };

  void setRegionName(std::string regionName) { m_regionName = regionName; };

  int compareTo(TraceContext o) { return (int)(m_timeStamp - o.getTimeStamp()); };

  std::string tostring() {
    std::string sb;
    if (m_traceBeans.size() > 0) {
      for (TraceBean bean : m_traceBeans) {
        sb.append(bean.getMsgId() + "_" + bean.getTopic() + "_");
      }
    }
    return "TraceContext{" + sb + '}';
  }
};

class TraceTransferBean {
 private:
  std::string m_transData;
  std::set<std::string> m_transKey;

 public:
  std::string getTransData() { return m_transData; }

  void setTransData(const std::string& transDatav) { m_transData = transDatav; }

  std::set<std::string> getTransKey() { return m_transKey; }

  void setTransKey(std::set<std::string> transKeyv) { m_transKey = transKeyv; }
};

class TraceDataEncoder {
 public:
  static std::list<TraceContext> decoderFromTraceDatastring(const std::string& traceData);
  static TraceTransferBean encoderFromContextBean(TraceContext* ctx);
};

}  // namespace rocketmq

#endif