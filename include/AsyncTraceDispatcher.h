#ifndef __AsyncTraceDispatcher_H__
#define __AsyncTraceDispatcher_H__


#include <string>
#include <thread>
#include <atomic>
#include <memory>
#include <condition_variable>

#include "DefaultMQProducer.h"


#include "TraceHelper.h"


#include "TraceDispatcher.h"
#include "ClientRPCHook.h"
#include <memory>
namespace rocketmq {

	

class AsyncTraceDispatcher : public TraceDispatcher,public enable_shared_from_this<AsyncTraceDispatcher> {
 private:
  //static InternalLogger log;  //= ClientLogger.getLog();
  int queueSize;
  int batchSize;
  int maxMsgSize;
 // trace message Producer
  //ThreadPoolExecutor traceExecutor;
  std::atomic<long> discardCount;
  //std::thread* worker;
  std::shared_ptr<std::thread> worker; 
 public:
  std::shared_ptr<DefaultMQProducer> traceProducer; 
  // private ArrayBlockingQueue<TraceContext> traceContextQueue;


public:
  std::mutex m_traceContextQueuenotEmpty_mutex;
  std::condition_variable m_traceContextQueuenotEmpty;
  std::list<TraceContext> traceContextQueue;

  //ArrayBlockingQueue<Runnable> appenderQueue;
  std::list<TraceContext> appenderQueue;

  std::mutex m_appenderQueuenotEmpty_mutex;
  std::condition_variable m_appenderQueuenotEmpty;

  std::thread* shutDownHook;
  bool stopped = false; /*
  DefaultMQProducerImpl hostProducer;
  DefaultMQPushConsumerImpl hostConsumer;
  ThreadLocalIndex sendWhichQueue = new ThreadLocalIndex();*/
  std::string dispatcherId;// = UUID.randomUUID().toString(
  std::string traceTopicName;
  std::atomic<bool> isStarted;  //= new Atomicbool(false);
  AccessChannel accessChannel;//  = AccessChannel.LOCAL;
  std::atomic<bool> delydelflag;

  public:
  AsyncTraceDispatcher(std::string traceTopicName, RPCHook* rpcHook);
   DefaultMQProducer* getAndCreateTraceProducer(/*RPCHook* rpcHook*/);

   virtual bool append(TraceContext* ctx);
   virtual void flush();
   virtual void shutdown();
   virtual void registerShutDownHook();
   virtual void removeShutdownHook();
   virtual void start(std::string nameSrvAddr, AccessChannel accessChannel = AccessChannel::LOCAL);
     
   virtual void setdelydelflag(bool v) { delydelflag = v; }
   bool getdelydelflag() { return delydelflag; }

  AccessChannel getAccessChannel() { return accessChannel; }

  void setAccessChannel(AccessChannel accessChannelv) { accessChannel = accessChannelv; }


  std::string& getTraceTopicName() { return traceTopicName; }

 
  void setTraceTopicName(std::string traceTopicNamev) { traceTopicName = traceTopicNamev; }
  bool getisStarted() { return isStarted.load();};
 
  DefaultMQProducer* getTraceProducer() { return traceProducer.get(); } /*


  DefaultMQProducerImpl getHostProducer() { return hostProducer; }

 
  void setHostProducer(DefaultMQProducerImpl hostProducer) { this.hostProducer = hostProducer; }

 
  DefaultMQPushConsumerImpl getHostConsumer() { return hostConsumer; }

 
  void setHostConsumer(DefaultMQPushConsumerImpl hostConsumer) { this.hostConsumer = hostConsumer; }*/
};







struct AsyncRunnable_run_context {
  //bool stopped;
  int batchSize;
  //AsyncTraceDispatcher* atd;
  std::shared_ptr<AsyncTraceDispatcher> atd;
  //DefaultMQProducer* traceProducer;
  std::string TraceTopicName;
  AsyncRunnable_run_context(bool stoppedv, int batchSizev,
	  //AsyncTraceDispatcher* atdv,
	  const std::shared_ptr<AsyncTraceDispatcher>& atdv,
	  const std::string& TraceTopicNamev
	  //, DefaultMQProducer* traceProducerv
  )
      : //stopped(stoppedv),
        batchSize(batchSizev),
        atd(atdv),
        TraceTopicName(TraceTopicNamev)  //, traceProducer(traceProducerv)
  {};

};






class AsyncAppenderRequest /*:public Runnable*/ {
 private:
	std::vector<TraceContext> contextList;
	DefaultMQProducer* traceProducer;
	AccessChannel accessChannel;
  std::string traceTopicName;

 public:
  AsyncAppenderRequest(std::vector<TraceContext>& contextList,
                       DefaultMQProducer* traceProducer,
                       AccessChannel accessChannel,
                       std::string& traceTopicName);
	void run();
  void sendTraceData(std::vector<TraceContext>& contextList);

private:
  void flushData(std::vector<TraceTransferBean> transBeanList, std::string dataTopic, std::string regionId);
  void sendTraceDataByMQ(std::vector<std::string> keySet, std::string data,
		std::string dataTopic, std::string regionId);
  std::set<std::string> tryGetMessageQueueBrokerSet(DefaultMQProducer* clientFactory,
     std::string topic);
};




	//std::set<std::string> tryGetMessageQueueBrokerSet(DefaultMQProducerImpl producer, std::string topic);
}  // namespace rocketmq



#endif