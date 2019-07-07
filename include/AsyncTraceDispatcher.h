#ifndef __AsyncTraceDispatcher_H__
#define __AsyncTraceDispatcher_H__

#include <atomic>
#include <condition_variable>
#include <memory>
#include <string>
#include <thread>

#include "DefaultMQProducer.h"

#include "TraceHelper.h"

#include <memory>
#include "ClientRPCHook.h"
#include "TraceDispatcher.h"
namespace rocketmq {

class AsyncTraceDispatcher : public TraceDispatcher, public enable_shared_from_this<AsyncTraceDispatcher> {
 private:
  // static InternalLogger log;  //= ClientLogger.getLog();
  int m_queueSize;
  int m_batchSize;
  int m_maxMsgSize;
  // trace message Producer
  // ThreadPoolExecutor traceExecutor;
  std::atomic<long> m_discardCount;
  // std::thread* worker;
  std::shared_ptr<std::thread> m_worker;

 public:
  std::shared_ptr<DefaultMQProducer> m_traceProducer;
  // private ArrayBlockingQueue<TraceContext> traceContextQueue;

 public:
  std::mutex m_traceContextQueuenotEmpty_mutex;
  std::condition_variable m_traceContextQueuenotEmpty;
  std::list<TraceContext> traceContextQueue;

  // ArrayBlockingQueue<Runnable> appenderQueue;
  std::list<TraceContext> appenderQueue;

  std::mutex m_appenderQueuenotEmpty_mutex;
  std::condition_variable m_appenderQueuenotEmpty;

  std::thread* m_shutDownHook;
  bool m_stopped = false;
  std::string m_dispatcherId;  // = UUID.randomUUID().toString(
  std::string m_traceTopicName;
  std::atomic<bool> m_isStarted;  //= new Atomicbool(false);
  AccessChannel m_accessChannel;  //  = AccessChannel.LOCAL;
  std::atomic<bool> m_delydelflag;

 public:
  AsyncTraceDispatcher(std::string traceTopicName, RPCHook* rpcHook);
  DefaultMQProducer* getAndCreateTraceProducer(/*RPCHook* rpcHook*/);

  virtual bool append(TraceContext* ctx);
  virtual void flush();
  virtual void shutdown();
  virtual void registerShutDownHook();
  virtual void removeShutdownHook();
  virtual void start(std::string nameSrvAddr, AccessChannel accessChannel = AccessChannel::LOCAL);

  virtual void setdelydelflag(bool v) { m_delydelflag = v; }
  bool getdelydelflag() { return m_delydelflag; }

  AccessChannel getAccessChannel() { return m_accessChannel; }

  void setAccessChannel(AccessChannel accessChannelv) { m_accessChannel = accessChannelv; }

  std::string& getTraceTopicName() { return m_traceTopicName; }

  void setTraceTopicName(std::string traceTopicNamev) { m_traceTopicName = traceTopicNamev; }
  bool getisStarted() { return m_isStarted.load(); };

  DefaultMQProducer* getTraceProducer() { return m_traceProducer.get(); }
};

struct AsyncRunnable_run_context {
  int batchSize;
  std::shared_ptr<AsyncTraceDispatcher> atd;
  std::string TraceTopicName;
  AsyncRunnable_run_context(bool stoppedv,
                            int batchSizev,
                            const std::shared_ptr<AsyncTraceDispatcher>& atdv,
                            const std::string& TraceTopicNamev)
      : batchSize(batchSizev), atd(atdv), TraceTopicName(TraceTopicNamev){};
};

class AsyncAppenderRequest {
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
  void sendTraceDataByMQ(std::vector<std::string> keySet,
                         std::string data,
                         std::string dataTopic,
                         std::string regionId);
  std::set<std::string> tryGetMessageQueueBrokerSet(DefaultMQProducer* clientFactory, std::string topic);
};

// std::set<std::string> tryGetMessageQueueBrokerSet(DefaultMQProducerImpl producer, std::string topic);
}  // namespace rocketmq

#endif