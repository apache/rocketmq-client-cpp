#include "TraceHelper.h"
#include "UtilAll.h"
#include "MQMessageListener.h"
#include"CommunicationMode.h"
#include <sstream>

namespace rocketmq {

std::string TraceConstants::TRACE_INSTANCE_NAME = "PID_CLIENT_INNER_TRACE_PRODUCER";
std::string TraceConstants::TRACE_TOPIC_PREFIX = "TRACE_DATA_";
std::string TraceConstants::TraceConstants_GROUP_NAME = "_INNER_TRACE_PRODUCER";
char TraceConstants::CONTENT_SPLITOR = (char)1;
char TraceConstants::FIELD_SPLITOR = (char)2;

std::string TraceBean::LOCAL_ADDRESS = UtilAll::getLocalAddress();
  

     std::string TraceContext2String() {
		 std::string sb;/*(1024);
        sb.append(traceType).append("_").append(groupName)
            .append("_").append(regionId).append("_").append(isSuccess).append("_");
        if (traceBeans != null && traceBeans.size() > 0) {
            for (TraceBean bean : traceBeans) {
                sb.append(bean.getMsgId() + "_" + bean.getTopic() + "_");
            }
        }*/
        return "TraceContext{" + sb+ '}';
    }




	std::string ConsumeStatus2str(ConsumeStatus p) {
      return p == ConsumeStatus::CONSUME_SUCCESS ? "CONSUME_SUCCESS" : "RECONSUME_LATER";
    }

	 

std::ostream& operator<<(std::ostream& os, TraceType traceType) {
      switch (traceType) {
		case TraceType::Pub:
			  return os << "Pub";
		case TraceType::SubBefore:
			  return os << "SubBefore";
		case TraceType::SubAfter:
          return os << "SubAfter";
          // omit default case to trigger compiler warning for missing cases
      }
      return os << static_cast<std::uint16_t>(traceType);
    }

    




/**
 * Encode/decode for Trace Data
 */

   std::list<TraceContext> TraceDataEncoder::decoderFromTraceDatastring(const std::string& traceData) {
        std::list<TraceContext> resList;/*
        if (traceData == null || traceData.length() <= 0) {
            return resList;
        }
        std::string[] contextList = traceData.split(std::string.valueOf(TraceConstants.FIELD_SPLITOR));
        for (std::string context : contextList) {
            std::string[] line = context.split(std::string.valueOf(TraceConstants::CONTENT_SPLITOR));
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
                pubContext.setTraceBeans(new Arraystd::list<TraceBean>(1));
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
                subBeforeContext.setTraceBeans(new Arraystd::list<TraceBean>(1));
                subBeforeContext.getTraceBeans().add(bean);
                resList.add(subBeforeContext);
            } else if (line[0].equals(TraceType.SubAfter.name())) {
                TraceContext subAfterContext = new TraceContext();
                subAfterContext.setTraceType(TraceType.SubAfter);
                subAfterContext.setRequestId(line[1]);
                TraceBean bean = new TraceBean();
                bean.setMsgId(line[2]);
                bean.setKeys(line[5]);
                subAfterContext.setTraceBeans(new Arraystd::list<TraceBean>(1));
                subAfterContext.getTraceBeans().add(bean);
                subAfterContext.setCostTime(Integer.parseInt(line[3]));
                subAfterContext.setSuccess(Boolean.parseBoolean(line[4]));
                if (line.length >= 7) {
                    // add the context type
                    subAfterContext.setContextCode(Integer.parseInt(line[6]));
                }
                resList.add(subAfterContext);
            }
        }*/
        return resList;
    }

    /**
     * Encoding the trace context into data std::strings and keyset sets
     *
     * @param ctx
     * @return
     */
    TraceTransferBean TraceDataEncoder::encoderFromContextBean(TraceContext* ctxp) {
      TraceTransferBean transferBean;
        if (ctxp == nullptr) {
			return transferBean;
        }

		TraceContext& ctx = *ctxp;
        std::string sb;
        std::stringstream ss;

        switch (ctx.getTraceType()) {
            case Pub: {
                TraceBean bean = ctx.getTraceBeans().front();
                //append the content of context and traceBean to transferBean's TransData
               
				
					ss<<ctx.getTraceType()<<TraceConstants::CONTENT_SPLITOR
                    <<ctx.getTimeStamp()<<TraceConstants::CONTENT_SPLITOR
                    <<ctx.getRegionId()<<TraceConstants::CONTENT_SPLITOR
                    <<ctx.getGroupName()<<TraceConstants::CONTENT_SPLITOR
                    <<bean.getTopic()<<TraceConstants::CONTENT_SPLITOR
                    <<bean.getMsgId()<<TraceConstants::CONTENT_SPLITOR
                    <<bean.getTags()<<TraceConstants::CONTENT_SPLITOR
                    <<bean.getKeys()<<TraceConstants::CONTENT_SPLITOR
                    <<bean.getStoreHost()<<TraceConstants::CONTENT_SPLITOR
                    <<bean.getBodyLength()<<TraceConstants::CONTENT_SPLITOR
                    <<ctx.getCostTime()<<TraceConstants::CONTENT_SPLITOR
                    <<bean.getMsgType()<<TraceConstants::CONTENT_SPLITOR
                    <<bean.getOffsetMsgId()<<TraceConstants::CONTENT_SPLITOR
                    <<ctx.isSuccess()<<TraceConstants::FIELD_SPLITOR;


					/*

					                sb.append(ctx.getTraceType()).append(TraceConstants::CONTENT_SPLITOR)//
                    .append(ctx.getTimeStamp()).append(TraceConstants::CONTENT_SPLITOR)//
                    .append(ctx.getRegionId()).append(TraceConstants::CONTENT_SPLITOR)//
                    .append(ctx.getGroupName()).append(TraceConstants::CONTENT_SPLITOR)//
                    .append(bean.getTopic()).append(TraceConstants::CONTENT_SPLITOR)//
                    .append(bean.getMsgId()).append(TraceConstants::CONTENT_SPLITOR)//
                    .append(bean.getTags()).append(TraceConstants::CONTENT_SPLITOR)//
                    .append(bean.getKeys()).append(TraceConstants::CONTENT_SPLITOR)//
                    .append(bean.getStoreHost()).append(TraceConstants::CONTENT_SPLITOR)//
                    .append(bean.getBodyLength()).append(TraceConstants::CONTENT_SPLITOR)//
                    .append(ctx.getCostTime()).append(TraceConstants::CONTENT_SPLITOR)//
                    .append(bean.getMsgType().ordinal()).append(TraceConstants::CONTENT_SPLITOR)//
                    .append(bean.getOffsetMsgId()).append(TraceConstants::CONTENT_SPLITOR)//
                    .append(ctx.isSuccess()).append(TraceConstants::FIELD_SPLITOR);*/
            }//case
            break;
            case SubBefore: { 
				





				
                for (TraceBean bean : ctx.getTraceBeans()) {
                ss << ctx.getTraceType() << TraceConstants::CONTENT_SPLITOR << ctx.getTimeStamp()
                   << TraceConstants::CONTENT_SPLITOR << ctx.getRegionId() << TraceConstants::CONTENT_SPLITOR
                     << ctx.getGroupName() << TraceConstants::CONTENT_SPLITOR << ctx.getRequestId()
                   << TraceConstants::CONTENT_SPLITOR << bean.getMsgId() << TraceConstants::CONTENT_SPLITOR
                   << bean.getRetryTimes() << TraceConstants::CONTENT_SPLITOR << bean.getKeys()
                   << TraceConstants::CONTENT_SPLITOR;
                    
					/*
                    sb.append(ctx.getTraceType()).append(TraceConstants::CONTENT_SPLITOR)//
                        .append(ctx.getTimeStamp()).append(TraceConstants::CONTENT_SPLITOR)//
                        .append(ctx.getRegionId()).append(TraceConstants::CONTENT_SPLITOR)//
                        .append(ctx.getGroupName()).append(TraceConstants::CONTENT_SPLITOR)//
                        .append(ctx.getRequestId()).append(TraceConstants::CONTENT_SPLITOR)//
                        .append(bean.getMsgId()).append(TraceConstants::CONTENT_SPLITOR)//
                        .append(bean.getRetryTimes()).append(TraceConstants::CONTENT_SPLITOR)//
                      .append(bean.getKeys()).append(TraceConstants::FIELD_SPLITOR);  //
					*/
                }//for
            }//case
            break;
            case SubAfter: {
              for (TraceBean bean : ctx.getTraceBeans()) {
                ss << ctx.getTraceType() << TraceConstants::CONTENT_SPLITOR << ctx.getRequestId()
                   << TraceConstants::CONTENT_SPLITOR << bean.getMsgId() << TraceConstants::CONTENT_SPLITOR
                   << ctx.getCostTime() << TraceConstants::CONTENT_SPLITOR << ctx.isSuccess()
                   << TraceConstants::CONTENT_SPLITOR << bean.getKeys() << TraceConstants::CONTENT_SPLITOR
                   << ctx.getContextCode() << TraceConstants::FIELD_SPLITOR;

                /*
                      sb.append(ctx.getTraceType()).append(TraceConstants::CONTENT_SPLITOR)//
                          .append(ctx.getRequestId()).append(TraceConstants::CONTENT_SPLITOR)//
                          .append(bean.getMsgId()).append(TraceConstants::CONTENT_SPLITOR)//
                          .append(ctx.getCostTime()).append(TraceConstants::CONTENT_SPLITOR)//
                          .append(ctx.isSuccess()).append(TraceConstants::CONTENT_SPLITOR)//
                          .append(bean.getKeys()).append(TraceConstants::CONTENT_SPLITOR)//
                        .append(ctx.getContextCode()).append(TraceConstants::FIELD_SPLITOR);

            */
              }//for
            }//case
            break;
            default:
              break;
        }//sw

        transferBean.setTransData(ss.str());
        for (TraceBean bean : ctx.getTraceBeans()) {
            transferBean.getTransKey().insert(bean.getMsgId());
            if (bean.getKeys() != null && bean.getKeys().length() > 0) {
              transferBean.getTransKey().insert(bean.getKeys());
            }
        }//for
		 return  transferBean;
    }







}