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
package org.apache.rocketmq.client.trace;

import org.apache.rocketmq.common.message.MessageType;

import java.util.ArrayList;
import java.util.List;

namespace rocketmq {

public class TraceBean {
    private static final String LOCAL_ADDRESS = UtilAll.ipToIPv4Str(UtilAll.getIP());
    private String topic = "";
    private String msgId = "";
    private String offsetMsgId = "";
    private String tags = "";
    private String keys = "";
    private String storeHost = LOCAL_ADDRESS;
    private String clientHost = LOCAL_ADDRESS;
    private long storeTime;
    private int retryTimes;
    private int bodyLength;
    private MessageType msgType;


    public MessageType getMsgType() {
        return msgType;
    }


    public void setMsgType(final MessageType msgType) {
        this.msgType = msgType;
    }


    public String getOffsetMsgId() {
        return offsetMsgId;
    }


    public void setOffsetMsgId(final String offsetMsgId) {
        this.offsetMsgId = offsetMsgId;
    }

    public String getTopic() {
        return topic;
    }


    public void setTopic(String topic) {
        this.topic = topic;
    }


    public String getMsgId() {
        return msgId;
    }


    public void setMsgId(String msgId) {
        this.msgId = msgId;
    }


    public String getTags() {
        return tags;
    }


    public void setTags(String tags) {
        this.tags = tags;
    }


    public String getKeys() {
        return keys;
    }


    public void setKeys(String keys) {
        this.keys = keys;
    }


    public String getStoreHost() {
        return storeHost;
    }


    public void setStoreHost(String storeHost) {
        this.storeHost = storeHost;
    }


    public String getClientHost() {
        return clientHost;
    }


    public void setClientHost(String clientHost) {
        this.clientHost = clientHost;
    }


    public long getStoreTime() {
        return storeTime;
    }


    public void setStoreTime(long storeTime) {
        this.storeTime = storeTime;
    }


    public int getRetryTimes() {
        return retryTimes;
    }


    public void setRetryTimes(int retryTimes) {
        this.retryTimes = retryTimes;
    }


    public int getBodyLength() {
        return bodyLength;
    }


    public void setBodyLength(int bodyLength) {
        this.bodyLength = bodyLength;
    }
}







public class TraceConstants {

    public static final String GROUP_NAME = "_INNER_TRACE_PRODUCER";
    public static final char CONTENT_SPLITOR = (char) 1;
    public static final char FIELD_SPLITOR = (char) 2;
    public static final String TRACE_INSTANCE_NAME = "PID_CLIENT_INNER_TRACE_PRODUCER";
    public static final String TRACE_TOPIC_PREFIX = MixAll.SYSTEM_TOPIC_PREFIX + "TRACE_DATA_";
}








public class TraceContext implements Comparable<TraceContext> {

    private TraceType traceType;
    private long timeStamp = System.currentTimeMillis();
    private String regionId = "";
    private String regionName = "";
    private String groupName = "";
    private int costTime = 0;
    private boolean isSuccess = true;
    private String requestId = MessageClientIDSetter.createUniqID();
    private int contextCode = 0;
    private List<TraceBean> traceBeans;

    public int getContextCode() {
        return contextCode;
    }

    public void setContextCode(final int contextCode) {
        this.contextCode = contextCode;
    }

    public List<TraceBean> getTraceBeans() {
        return traceBeans;
    }

    public void setTraceBeans(List<TraceBean> traceBeans) {
        this.traceBeans = traceBeans;
    }

    public String getRegionId() {
        return regionId;
    }

    public void setRegionId(String regionId) {
        this.regionId = regionId;
    }

    public TraceType getTraceType() {
        return traceType;
    }

    public void setTraceType(TraceType traceType) {
        this.traceType = traceType;
    }

    public long getTimeStamp() {
        return timeStamp;
    }

    public void setTimeStamp(long timeStamp) {
        this.timeStamp = timeStamp;
    }

    public String getGroupName() {
        return groupName;
    }

    public void setGroupName(String groupName) {
        this.groupName = groupName;
    }

    public int getCostTime() {
        return costTime;
    }

    public void setCostTime(int costTime) {
        this.costTime = costTime;
    }

    public boolean isSuccess() {
        return isSuccess;
    }

    public void setSuccess(boolean success) {
        isSuccess = success;
    }

    public String getRequestId() {
        return requestId;
    }

    public void setRequestId(String requestId) {
        this.requestId = requestId;
    }

    public String getRegionName() {
        return regionName;
    }

    public void setRegionName(String regionName) {
        this.regionName = regionName;
    }

    @Override
    public int compareTo(TraceContext o) {
        return (int) (this.timeStamp - o.getTimeStamp());
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder(1024);
        sb.append(traceType).append("_").append(groupName)
            .append("_").append(regionId).append("_").append(isSuccess).append("_");
        if (traceBeans != null && traceBeans.size() > 0) {
            for (TraceBean bean : traceBeans) {
                sb.append(bean.getMsgId() + "_" + bean.getTopic() + "_");
            }
        }
        return "TraceContext{" + sb.toString() + '}';
    }
}













public interface TraceDispatcher {

    /**
     * Initialize asynchronous transfer data module
     */
    void start(String nameSrvAddr, AccessChannel accessChannel) throws MQClientException;

    /**
     * Append the transfering data
     * @param ctx data infomation
     * @return
     */
    boolean append(Object ctx);

    /**
     * Write flush action
     *
     * @throws IOException
     */
    void flush() throws IOException;

    /**
     * Close the trace Hook
     */
    void shutdown();
}



public enum TraceDispatcherType {
    PRODUCER,
    CONSUMER
}









public class TraceTransferBean {
    private String transData;
    private Set<String> transKey = new HashSet<String>();

    public String getTransData() {
        return transData;
    }

    public void setTransData(String transData) {
        this.transData = transData;
    }

    public Set<String> getTransKey() {
        return transKey;
    }

    public void setTransKey(Set<String> transKey) {
        this.transKey = transKey;
    }
}










public enum TraceType {
    Pub,
    SubBefore,
    SubAfter,
}

















/**
 * Encode/decode for Trace Data
 */
public class TraceDataEncoder {

    /**
     * Resolving traceContext list From trace data String
     *
     * @param traceData
     * @return
     */
    public static List<TraceContext> decoderFromTraceDataString(String traceData) {
        List<TraceContext> resList = new ArrayList<TraceContext>();
        if (traceData == null || traceData.length() <= 0) {
            return resList;
        }
        String[] contextList = traceData.split(String.valueOf(TraceConstants.FIELD_SPLITOR));
        for (String context : contextList) {
            String[] line = context.split(String.valueOf(TraceConstants.CONTENT_SPLITOR));
            if (line[0].equals(TraceType.Pub.name())) {
                TraceContext pubContext = new TraceContext();
                pubContext.setTraceType(TraceType.Pub);
                pubContext.setTimeStamp(Long.parseLong(line[1]));
                pubContext.setRegionId(line[2]);
                pubContext.setGroupName(line[3]);
                TraceBean bean = new TraceBean();
                bean.setTopic(line[4]);
                bean.setMsgId(line[5]);
                bean.setTags(line[6]);
                bean.setKeys(line[7]);
                bean.setStoreHost(line[8]);
                bean.setBodyLength(Integer.parseInt(line[9]));
                pubContext.setCostTime(Integer.parseInt(line[10]));
                bean.setMsgType(MessageType.values()[Integer.parseInt(line[11])]);

                if (line.length == 13) {
                    pubContext.setSuccess(Boolean.parseBoolean(line[12]));
                } else if (line.length == 14) {
                    bean.setOffsetMsgId(line[12]);
                    pubContext.setSuccess(Boolean.parseBoolean(line[13]));
                }
                pubContext.setTraceBeans(new ArrayList<TraceBean>(1));
                pubContext.getTraceBeans().add(bean);
                resList.add(pubContext);
            } else if (line[0].equals(TraceType.SubBefore.name())) {
                TraceContext subBeforeContext = new TraceContext();
                subBeforeContext.setTraceType(TraceType.SubBefore);
                subBeforeContext.setTimeStamp(Long.parseLong(line[1]));
                subBeforeContext.setRegionId(line[2]);
                subBeforeContext.setGroupName(line[3]);
                subBeforeContext.setRequestId(line[4]);
                TraceBean bean = new TraceBean();
                bean.setMsgId(line[5]);
                bean.setRetryTimes(Integer.parseInt(line[6]));
                bean.setKeys(line[7]);
                subBeforeContext.setTraceBeans(new ArrayList<TraceBean>(1));
                subBeforeContext.getTraceBeans().add(bean);
                resList.add(subBeforeContext);
            } else if (line[0].equals(TraceType.SubAfter.name())) {
                TraceContext subAfterContext = new TraceContext();
                subAfterContext.setTraceType(TraceType.SubAfter);
                subAfterContext.setRequestId(line[1]);
                TraceBean bean = new TraceBean();
                bean.setMsgId(line[2]);
                bean.setKeys(line[5]);
                subAfterContext.setTraceBeans(new ArrayList<TraceBean>(1));
                subAfterContext.getTraceBeans().add(bean);
                subAfterContext.setCostTime(Integer.parseInt(line[3]));
                subAfterContext.setSuccess(Boolean.parseBoolean(line[4]));
                if (line.length >= 7) {
                    // add the context type
                    subAfterContext.setContextCode(Integer.parseInt(line[6]));
                }
                resList.add(subAfterContext);
            }
        }
        return resList;
    }

    /**
     * Encoding the trace context into data strings and keyset sets
     *
     * @param ctx
     * @return
     */
    public static TraceTransferBean encoderFromContextBean(TraceContext ctx) {
        if (ctx == null) {
            return null;
        }
        //build message trace of the transfering entity content bean
        TraceTransferBean transferBean = new TraceTransferBean();
        StringBuilder sb = new StringBuilder(256);
        switch (ctx.getTraceType()) {
            case Pub: {
                TraceBean bean = ctx.getTraceBeans().get(0);
                //append the content of context and traceBean to transferBean's TransData
                sb.append(ctx.getTraceType()).append(TraceConstants.CONTENT_SPLITOR)//
                    .append(ctx.getTimeStamp()).append(TraceConstants.CONTENT_SPLITOR)//
                    .append(ctx.getRegionId()).append(TraceConstants.CONTENT_SPLITOR)//
                    .append(ctx.getGroupName()).append(TraceConstants.CONTENT_SPLITOR)//
                    .append(bean.getTopic()).append(TraceConstants.CONTENT_SPLITOR)//
                    .append(bean.getMsgId()).append(TraceConstants.CONTENT_SPLITOR)//
                    .append(bean.getTags()).append(TraceConstants.CONTENT_SPLITOR)//
                    .append(bean.getKeys()).append(TraceConstants.CONTENT_SPLITOR)//
                    .append(bean.getStoreHost()).append(TraceConstants.CONTENT_SPLITOR)//
                    .append(bean.getBodyLength()).append(TraceConstants.CONTENT_SPLITOR)//
                    .append(ctx.getCostTime()).append(TraceConstants.CONTENT_SPLITOR)//
                    .append(bean.getMsgType().ordinal()).append(TraceConstants.CONTENT_SPLITOR)//
                    .append(bean.getOffsetMsgId()).append(TraceConstants.CONTENT_SPLITOR)//
                    .append(ctx.isSuccess()).append(TraceConstants.FIELD_SPLITOR);
            }
            break;
            case SubBefore: {
                for (TraceBean bean : ctx.getTraceBeans()) {
                    sb.append(ctx.getTraceType()).append(TraceConstants.CONTENT_SPLITOR)//
                        .append(ctx.getTimeStamp()).append(TraceConstants.CONTENT_SPLITOR)//
                        .append(ctx.getRegionId()).append(TraceConstants.CONTENT_SPLITOR)//
                        .append(ctx.getGroupName()).append(TraceConstants.CONTENT_SPLITOR)//
                        .append(ctx.getRequestId()).append(TraceConstants.CONTENT_SPLITOR)//
                        .append(bean.getMsgId()).append(TraceConstants.CONTENT_SPLITOR)//
                        .append(bean.getRetryTimes()).append(TraceConstants.CONTENT_SPLITOR)//
                        .append(bean.getKeys()).append(TraceConstants.FIELD_SPLITOR);//
                }
            }
            break;
            case SubAfter: {
                for (TraceBean bean : ctx.getTraceBeans()) {
                    sb.append(ctx.getTraceType()).append(TraceConstants.CONTENT_SPLITOR)//
                        .append(ctx.getRequestId()).append(TraceConstants.CONTENT_SPLITOR)//
                        .append(bean.getMsgId()).append(TraceConstants.CONTENT_SPLITOR)//
                        .append(ctx.getCostTime()).append(TraceConstants.CONTENT_SPLITOR)//
                        .append(ctx.isSuccess()).append(TraceConstants.CONTENT_SPLITOR)//
                        .append(bean.getKeys()).append(TraceConstants.CONTENT_SPLITOR)//
                        .append(ctx.getContextCode()).append(TraceConstants.FIELD_SPLITOR);
                }
            }
            break;
            default:
        }
        transferBean.setTransData(sb.toString());
        for (TraceBean bean : ctx.getTraceBeans()) {

            transferBean.getTransKey().add(bean.getMsgId());
            if (bean.getKeys() != null && bean.getKeys().length() > 0) {
                transferBean.getTransKey().add(bean.getKeys());
            }
        }
        return transferBean;
    }
}











}