#ifndef __ConsumeMessageContext_H__
#define __ConsumeMessageContext_H__
#include <string>
#include <list>
#include <map>


#include "MQMessageExt.h"
#include "MQMessageQueue.h"
#include "TraceHelper.h"

namespace rocketmq {

class ConsumeMessageContext {
 private:
  std::string consumerGroup;
  std::list<MQMessageExt> msgList;
  MQMessageQueue mq;
  bool success;
  std::string status;
  TraceContext* mqTraceContext;
  std::map<std::string, std::string> props;
  std::string msgnamespace;

 public:
  std::string getConsumerGroup() { return consumerGroup; };

  void setConsumerGroup(std::string consumerGroup) { consumerGroup = consumerGroup; };

  std::list<MQMessageExt> getMsgList() { return msgList; };

  void setMsgList(std::list<MQMessageExt> msgList) { msgList = msgList; };
  void setMsgList(std::vector<MQMessageExt> pmsgList) { msgList.assign(pmsgList.begin(), pmsgList.end());
  };
  MQMessageQueue getMq() { return mq; };

  void setMq(MQMessageQueue mq) { mq = mq; };

  bool isSuccess() { return success; };

  void setSuccess(bool success) { success = success; };

  TraceContext* getMqTraceContext() { return mqTraceContext; };

  void setMqTraceContext(TraceContext* pmqTraceContext) { mqTraceContext = pmqTraceContext; };

  std::map<std::string, std::string> getProps() { return props; };

  void setProps(std::map<std::string, std::string> props) { props = props; };

  std::string getStatus() { return status; };

  void setStatus(std::string status) { status = status; };

  std::string getNamespace() { return msgnamespace; };

  void setNamespace(std::string msgnamespace) { msgnamespace = msgnamespace; };
};

}  // namespace rocketmq

#endif  //__ConsumeMessageContext_H__