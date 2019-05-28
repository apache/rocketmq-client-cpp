#ifndef __TraceDispatcher_H__
#define __TraceDispatcher_H__


#include <string>
#include "TraceHelper.h"

namespace rocketmq {

enum AccessChannel {
  /**
   * Means connect to private IDC cluster.
   */
  LOCAL,

  /**
   * Means connect to Cloud service.
   */
  CLOUD,
};
	


class TraceDispatcher {
public:
  virtual void start(std::string nameSrvAddr, AccessChannel accessChannel = AccessChannel::LOCAL){};

  virtual bool append(TraceContext* ctx) { return true; };

  virtual void flush(){};

  virtual void shutdown(){};

  virtual void setdelydelflag(bool v){};

};



}  // namespace rocketmq



#endif