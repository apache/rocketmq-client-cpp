#ifndef __SendMessageTraceHookImpl_H__
#define __SendMessageTraceHookImpl_H__

#include <memory>
#include "SendMessageHook.h"
namespace rocketmq {
class SendMessageTraceHookImpl : public SendMessageHook {
 private:
  std::shared_ptr<TraceDispatcher> localDispatcher;

  public:
  SendMessageTraceHookImpl(std::shared_ptr<TraceDispatcher>& localDispatcher);
  virtual std::string hookName();
  //virtual void sendMessageBefore(SendMessageContext* context);
  virtual void sendMessageBefore(SendMessageContext& context);
  virtual void sendMessageAfter(SendMessageContext& context);
  //virtual void sendMessageAfter(SendMessageContext* context);
};





}






#endif