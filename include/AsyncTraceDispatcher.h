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
#ifndef __ASYNCTRACEDISPATCHER_H__
#define __ASYNCTRACEDISPATCHER_H__

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
  int m_queueSize;
  int m_batchSize;
  int m_maxMsgSize;
  std::atomic<long> m_discardCount;
  std::shared_ptr<std::thread> m_worker;

  // public:
  std::shared_ptr<DefaultMQProducer> m_traceProducer;

  // public:
  std::mutex m_traceContextQueuenotEmpty_mutex;
  std::condition_variable m_traceContextQueuenotEmpty;
  std::list<TraceContext> m_traceContextQueue;

  std::list<TraceContext> m_appenderQueue;

  std::mutex m_appenderQueuenotEmpty_mutex;
  std::condition_variable m_appenderQueuenotEmpty;

  std::thread* m_shutDownHook;
  bool m_stopped;
  std::string m_dispatcherId;
  std::string m_traceTopicName;
  std::atomic<bool> m_isStarted;
  AccessChannel m_accessChannel;
  std::atomic<bool> m_delydelflag;

 public:
  bool get_stopped() { return m_stopped; }
  bool tryPopFrontContext(TraceContext& context);
  AsyncTraceDispatcher(std::string traceTopicName, RPCHook* rpcHook);
  DefaultMQProducer* getAndCreateTraceProducer();

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

 std::shared_ptr<DefaultMQProducer> getTraceProducer() { return m_traceProducer; }
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
  //DefaultMQProducer* traceProducer;
  std::shared_ptr<DefaultMQProducer> traceProducer;
  AccessChannel accessChannel;
  std::string traceTopicName;

 public:
  AsyncAppenderRequest(std::vector<TraceContext>& contextList,
                       //DefaultMQProducer* traceProducer,
                       std::shared_ptr<DefaultMQProducer> traceProducerv,
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
  std::set<std::string> tryGetMessageQueueBrokerSet(std::shared_ptr<DefaultMQProducer> producer, std::string topic);
};

}  // namespace rocketmq

#endif