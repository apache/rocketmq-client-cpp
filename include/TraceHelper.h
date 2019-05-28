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
/*
package org.apache.rocketmq.client.trace;

import org.apache.rocketmq.common.message.MessageType;

import java.util.ArrayList;
import java.util.List;
*/
#ifndef __Tracehelper_H__
#define __Tracehelper_H__
#include <set>
#include <string>
#include <list>


namespace rocketmq {


//typedef int CommunicationMode;


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
  static std::string LOCAL_ADDRESS;  //= "/*UtilAll.ipToIPv4Str(UtilAll.getIP())*/";
  
 private:
  std::string topic = "";
  std::string msgId = "";
  std::string offsetMsgId = "";
  std::string tags = "";
  std::string keys = "";
  std::string storeHost;//  = LOCAL_ADDRESS;
  std::string clientHost;//  = LOCAL_ADDRESS;
  long storeTime;
  int retryTimes;
  int bodyLength;
  MessageType msgType;

 public:
  MessageType getMsgType() { return msgType; };

  void setMsgType(MessageType msgTypev) { msgType = msgTypev; };

  std::string getOffsetMsgId() { return offsetMsgId; };

  void setOffsetMsgId(std::string offsetMsgIdv) { offsetMsgId = offsetMsgIdv; };

  std::string getTopic() { return topic; };

  void setTopic(std::string topicv) { topic = topicv; };

  std::string getMsgId() { return msgId; };

  void setMsgId(std::string msgIdv) { msgId = msgIdv; };

  std::string getTags() { return tags; };

  void setTags(std::string tagsv) { tags = tagsv; };

  std::string getKeys() { return keys; };

  void setKeys(std::string keysv) { keys = keysv; }

  std::string getStoreHost() { return storeHost; };

  void setStoreHost(std::string storeHostv) { storeHost = storeHostv; };

  std::string getClientHost() { return clientHost; };

  void setClientHost(std::string clientHostv) { clientHost = clientHostv; };

  long getStoreTime() { return storeTime; };

  void setStoreTime(long storeTimev) { storeTime = storeTimev; };

  int getRetryTimes() { return retryTimes; };

  void setRetryTimes(int retryTimesv) { retryTimes = retryTimesv; };

  int getBodyLength() { return bodyLength; };

  void setBodyLength(int bodyLengthv) { bodyLength = bodyLengthv; };
};

class TraceConstants {
 public:
  static std::string TraceConstants_GROUP_NAME;  // = "_INNER_TRACE_PRODUCER";
  static char CONTENT_SPLITOR;//  = (char)1;
  static char FIELD_SPLITOR;  // = (char)2;
  static std::string TRACE_INSTANCE_NAME;  // = "PID_CLIENT_INNER_TRACE_PRODUCER";
  static std::string TRACE_TOPIC_PREFIX;  // = MixAll.SYSTEM_TOPIC_PREFIX + "TRACE_DATA_";
};

class TraceContext /*: public Comparable<TraceContext>*/ {
 private:
  TraceType traceType;
  long timeStamp;  //= System.currentTimeMillis();
  std::string regionId = "";
  std::string regionName = "";
  std::string groupName = "";
  int costTime = 0;
  bool misSuccess = true;
  std::string requestId;  //= MessageClientIDSetter.createUniqID();
  int contextCode = 0;
  std::list<TraceBean> traceBeans;

 public:
  int getContextCode() { return contextCode; };

  void setContextCode(int contextCodev) { contextCode = contextCodev; };

  std::list<TraceBean>& getTraceBeans() { return traceBeans; };

  void setTraceBeans(std::list<TraceBean> traceBeansv) { traceBeans = traceBeansv; };

  std::string getRegionId() { return regionId; };

  void setRegionId(std::string regionIdv) { regionId = regionIdv; };

  TraceType getTraceType() { return traceType; };

  void setTraceType(TraceType traceTypev) { traceType = traceTypev; };

  long getTimeStamp() { return timeStamp; };

  void setTimeStamp(long timeStampv) { timeStamp = timeStampv; };

  std::string getGroupName() { return groupName; };

  void setGroupName(std::string groupNamev) { groupName = groupNamev; };

  int getCostTime() { return costTime; };

  void setCostTime(int costTimev) { costTime = costTimev; };

  bool isSuccess() { return misSuccess; };

  void setSuccess(bool successv) { misSuccess = successv; };

  std::string getRequestId() { return requestId; };

  void setRequestId(std::string requestIdv) { requestId = requestIdv; };

  std::string getRegionName() { return regionName; };

  void setRegionName(std::string regionName) { regionName = regionName; };

  int compareTo(TraceContext o) {
    return (int)(timeStamp - o.getTimeStamp());
  };

  std::string tostring() {
    std::string sb;//    = new std::stringBuilder(1024);
    /*sb.append( ((char)traceType) )
        .append("_")
        .append(groupName)
        .append("_")
        .append(regionId)
        .append("_")
        .append(misSuccess)
        .append("_");*/
    if (/*traceBeans != nullptr &&*/ traceBeans.size() > 0) {
      for (TraceBean bean : traceBeans) {
        sb.append(bean.getMsgId() + "_" + bean.getTopic() + "_");
      }
    }
    return "TraceContext{" + sb + '}';
  }
};



class TraceTransferBean {
 private:
  std::string transData;
  std::set<std::string> transKey; //  = new std::set<std::string>();

 public:
  std::string getTransData() { return transData; }

  void setTransData(std::string& transDatav) { transData = transDatav; }

  std::set<std::string> getTransKey() { return transKey; }

  void setTransKey(std::set<std::string> transKeyv) { transKey = transKeyv; }
};







class TraceDataEncoder{
public:
	static std::list<TraceContext> decoderFromTraceDatastring(std::string& traceData);
	static TraceTransferBean encoderFromContextBean(TraceContext* ctx);

};





}  // namespace rocketmq



#endif