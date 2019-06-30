#include "SendMessageTraceHookImpl.h"
#include "SendMessageHook.h"

#include "Logging.h"
#include <mutex>
#include <chrono>


namespace rocketmq {

SendMessageTraceHookImpl::SendMessageTraceHookImpl(std::shared_ptr<TraceDispatcher>& localDispatcherv) {
  localDispatcher = std::shared_ptr<TraceDispatcher> (localDispatcherv);
}

std::string SendMessageTraceHookImpl::hookName() {
  return "SendMessageTraceHook";
}

void SendMessageTraceHookImpl::sendMessageBefore(SendMessageContext& context) {

  // if it is message trace data,then it doesn't recorded
   /*if (nullptr||
context->getMessage().getTopic().startsWith(
((AsyncTraceDispatcher) localDispatcher).getTraceTopicName())

  ) {
    return;
  }*/
  // build the context content of TuxeTraceContext
  TraceContext* tuxeContext = new TraceContext();
  // tuxeContext->setTraceBeans(std::list<TraceBean>(1));
  context.setMqTraceContext(tuxeContext);
  tuxeContext->setTraceType(TraceType::Pub);
  tuxeContext->setGroupName(context.getProducerGroup());
  // build the data bean object of message trace
  TraceBean traceBean;
  traceBean.setTopic(context.getMessage().getTopic());
  traceBean.setTags(context.getMessage().getTags());
  traceBean.setKeys(context.getMessage().getKeys());
  traceBean.setStoreHost(context.getBrokerAddr());
  traceBean.setBodyLength(context.getMessage().getBody().length());
  traceBean.setMsgType(context.getMsgType());
  tuxeContext->getTraceBeans().push_back(traceBean);
}

void SendMessageTraceHookImpl::sendMessageAfter(SendMessageContext& context) {
  
  // if it is message trace data,then it doesn't recorded
  /*if (nullptr ||
  //|| context->getMessage().getTopic().startsWith(((AsyncTraceDispatcher) localDispatcher).getTraceTopicName()) ||
        context.getMqTraceContext() == nullptr) {
        return;
    }*/
  if (context.getSendResult() == nullptr) {
        return;
    }

  /*
    if (context.getSendResult()->getRegionId().compare("")==0 || !context.getSendResult()->isTraceOn()) {
        // if switch is false,skip it
        return;
    }*/

  TraceContext* tuxeContext = (TraceContext*)context.getMqTraceContext();
  TraceBean traceBean = tuxeContext->getTraceBeans().front();
  int costTime = 0;  //        (int)((System.currentTimeMillis() - tuxeContext.getTimeStamp()) /
                     //        tuxeContext.getTraceBeans().size());
  /*
  std::chrono::steady_clock::duration d = std::chrono::steady_clock::now().time_since_epoch();
  std::chrono::milliseconds mil = std::chrono::duration_cast<std::chrono::milliseconds>(d);
  costTime = mil.count() - tuxeContext->getTimeStamp();*/
  costTime = time(0) - tuxeContext->getTimeStamp();
  tuxeContext->setCostTime(costTime);
  if (context.getSendResult()->getSendStatus() == (SendStatus::SEND_OK)) {
    tuxeContext->setSuccess(true);
  } else {
    tuxeContext->setSuccess(false);
  }
  std::string regionId = context.getMessage().getProperty(MQMessage::PROPERTY_MSG_REGION);
  std::string traceOn = context.getMessage().getProperty(MQMessage::PROPERTY_TRACE_SWITCH);

  tuxeContext->setRegionId(regionId);
  traceBean.setMsgId(context.getSendResult()->getMsgId());
  traceBean.setOffsetMsgId(context.getSendResult()->getOffsetMsgId());
  traceBean.setStoreTime(tuxeContext->getTimeStamp() + costTime / 2);

	{
	  localDispatcher->append(tuxeContext);
    LOG_INFO("SendMessageTraceHookImpl::sendMessageAfter append");
	}
}

}  // namespace rocketmq