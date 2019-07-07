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
#include "AsyncTraceDispatcher.h"
#include <chrono>
#include "ClientRPCHook.h"
#include "Logging.h"

//#include "MQClient.h"
#include "MQClientFactory.h"
#include "MQSelector.h"
#include "ThreadLocalIndex.h"
#include "TopicPublishInfo.h"
#include "UtilAll.h"

#include <chrono>
#include <cmath>

namespace rocketmq {

class TraceMessageQueueSelector : public MessageQueueSelector {
 private:
  ThreadLocalIndex tli;

 public:
  MQMessageQueue select(const std::vector<MQMessageQueue>& mqs, const MQMessage& msg, void* arg) {
    std::set<std::string>* brokerSet = (std::set<std::string>*)arg;
    std::vector<MQMessageQueue> filterMqs;
    for (auto& queue : mqs) {
      if (brokerSet->end() != brokerSet->find(queue.getBrokerName())) {
        filterMqs.push_back(queue);
      }
    }
    int index = tli.getAndIncrement();
    int pos = std::abs(index) % filterMqs.size();
    if (pos < 0) {
      pos = 0;
    }
    return filterMqs.at(pos);
  }
};

class TraceMessageSendCallback : public SendCallback {
  virtual void onSuccess(SendResult& sendResult) {
    ;
    LOG_INFO("send trace data ,onSuccess,the traceData" /*+ data.cstr()*/);
  }
  virtual void onException(MQException& e) {
    LOG_INFO("send trace data ,onException,the traceData" /*+ data.cstr()*/);
    ;
  }
};

void AsyncRunnable_run(AsyncRunnable_run_context* ctx);

AsyncTraceDispatcher::AsyncTraceDispatcher(std::string traceTopicNamev, RPCHook* rpcHook) {
  // queueSize is greater than or equal to the n power of 2 of value
  m_queueSize = 2048;
  m_batchSize = 100;
  m_maxMsgSize = 128000;
  m_traceProducer = std::shared_ptr<DefaultMQProducer>(
      new DefaultMQProducer(TraceConstants::TraceConstants_GROUP_NAME, true, (void*)rpcHook));
  // traceProducer->setSessionCredentials(rpcHook->s)

  m_isStarted = false;
  m_discardCount = ATOMIC_VAR_INIT(0);
  m_delydelflag = ATOMIC_VAR_INIT(false);
  if (!UtilAll::isBlank(traceTopicNamev)) {
    m_traceTopicName = traceTopicNamev;
  } else {
    m_traceTopicName = RMQ_SYS_TRACE_TOPIC;
  }
}

void AsyncTraceDispatcher::start(std::string nameSrvAddr, AccessChannel accessChannel)  // throws MQClientException
{
  LOG_INFO("AsyncTraceDispatcher:%s start", nameSrvAddr.c_str());

  bool NotStated = false;
  if (m_isStarted.compare_exchange_weak(NotStated, true) == true) {
    m_traceProducer->setNamesrvAddr(nameSrvAddr);
    m_traceProducer->setInstanceName(TraceConstants::TRACE_INSTANCE_NAME + "_" + nameSrvAddr);
    m_traceProducer->start();
  }
  accessChannel = accessChannel;

  AsyncRunnable_run_context* arctx = new AsyncRunnable_run_context(false, 1, shared_from_this(), getTraceTopicName());
  if (m_worker.use_count() == 0) {
    m_worker = std::shared_ptr<std::thread>(new std::thread(&AsyncRunnable_run, arctx));
  }

  m_worker->detach();
}

DefaultMQProducer* AsyncTraceDispatcher::getAndCreateTraceProducer(/*RPCHook* rpcHook*/) {
  DefaultMQProducer* traceProducerInstance = m_traceProducer.get();
  if (traceProducerInstance == nullptr) {
    traceProducerInstance = new DefaultMQProducer(TraceConstants::TraceConstants_GROUP_NAME);  //"rpcHook");
    // traceProducerInstance->setProducerGroup(TraceConstants::GROUP_NAME);
    traceProducerInstance->setSendMsgTimeout(5000);
    // traceProducerInstance->setVipChannelEnabled(false);
    // The max size of message is 128K
    traceProducerInstance->setMaxMessageSize(m_maxMsgSize - 10 * 1000);
  }
  return traceProducerInstance;
}

bool AsyncTraceDispatcher::append(TraceContext* ctx) {
  {
    // printf("AsyncTraceDispatcher append\n");
    // TLock lock(m_mutex);
    std::unique_lock<std::mutex> lock(m_traceContextQueuenotEmpty_mutex);
    traceContextQueue.push_back(*ctx);
  }
  bool result = true;
  return result;
}

#ifdef WIN32
#define sleep(x) Sleep((x)*1000)
#endif

void AsyncTraceDispatcher::flush() {
  // The maximum waiting time for refresh,avoid being written all the time, resulting in failure to return.
  // long end = 0;  //    System.currentTimeMillis() + 500;
  auto end = std::chrono::system_clock::now() + std::chrono::milliseconds(500);

  while (traceContextQueue.size() > 0 || appenderQueue.size() > 0 && std::chrono::system_clock::now() <= end) {
    try {
      // Thread.sleep(1);
      sleep(1);
    } catch (InterruptedException e) {
      break;
    }
  }
  LOG_INFO("------end trace send  traceContextQueue.size() appenderQueue.size()");
}

void AsyncTraceDispatcher::shutdown() {
  m_stopped = true;
}

void AsyncTraceDispatcher::registerShutDownHook() {
  if (m_shutDownHook == nullptr) {
    /*shutDownHook = new Thread(new Runnable() {
      private volatile boolean hasShutdown = false;

      @Override
      public void run() {
        synchronized (this) {
          if (!hasShutdown) {
            try {
              flush();
            } catch (IOException e) {
              log.error("system MQTrace hook shutdown failed ,maybe loss some trace data");
            }
          }
        }
      }
    }, "ShutdownHookMQTrace");
    Runtime.getRuntime().addShutdownHook(shutDownHook);*/
  }
}

void AsyncTraceDispatcher::removeShutdownHook() {
  if (m_shutDownHook != nullptr) {
  }
}

void AsyncRunnable_run(AsyncRunnable_run_context* ctx) {
  while (!ctx->atd->m_stopped) {
    std::vector<TraceContext> contexts;
    LOG_INFO("AsyncRunnable_run:TraceContext fetch ctx->atd->traceContextQueue %d", ctx->atd->traceContextQueue.size());
    for (int i = 0; i < ctx->batchSize; i++) {
      try {
        {
          std::unique_lock<std::mutex> lock(ctx->atd->m_traceContextQueuenotEmpty_mutex);
          if (ctx->atd->traceContextQueue.empty()) {
            ctx->atd->m_traceContextQueuenotEmpty.wait_for(lock, std::chrono::seconds(5));
          }
          if (!ctx->atd->traceContextQueue.empty()) {
            contexts.push_back(ctx->atd->traceContextQueue.front());
            ctx->atd->traceContextQueue.pop_front();
          }
        }  // lock scope

      }  // try
      catch (InterruptedException e) {
        ;
      }

    }  // for

    LOG_INFO("AsyncRunnable_run:TraceContext fetchs end %d", contexts.size());
    if (contexts.size() > 0) {
      std::shared_ptr<AsyncAppenderRequest> request = std::shared_ptr<AsyncAppenderRequest>(new AsyncAppenderRequest(
          contexts, ctx->atd->m_traceProducer.get(), ctx->atd->m_accessChannel, ctx->TraceTopicName));
      // traceExecutor.submit(request);
      request->run();
    } else if (ctx->atd->m_stopped) {
      if (ctx->atd->getdelydelflag() == true) {
        delete ctx;
      }
    }
  }  // while
}  // foo

AsyncAppenderRequest::AsyncAppenderRequest(std::vector<TraceContext>& contextListv,
                                           DefaultMQProducer* traceProducerv,
                                           AccessChannel accessChannelv,
                                           std::string& traceTopicNamev) {
  if (!contextListv.empty()) {
    // contextList.assign(contextListv.begin(), contextListv.end());
    contextList = contextListv;
    traceProducer = traceProducerv;
    accessChannel = accessChannelv;
    traceTopicName = traceTopicNamev;
  }
}

void AsyncAppenderRequest::run() {
  sendTraceData(contextList);
}

void AsyncAppenderRequest::sendTraceData(std::vector<TraceContext>& contextListv) {
  LOG_INFO("sendTraceData:s start");

  std::map<std::string, std::vector<TraceTransferBean>> transBeanMap;

  for (auto& context : contextListv) {
    if (context.getTraceBeans().empty()) {
      continue;
    }
    // Topic value corresponding to original message entity content
    std::string topic = context.getTraceBeans().front().getTopic();
    std::string regionId = context.getRegionId();
    // Use  original message entity's topic as key
    std::string key = topic;
    if (!regionId.empty()) {
      key = key + TraceConstants::CONTENT_SPLITOR + regionId;
    }

    if (transBeanMap.find(key) == transBeanMap.end()) {
      std::vector<TraceTransferBean> transBeanList;
      transBeanMap.insert(std::make_pair(key, transBeanList));
    }
    auto it = transBeanMap.find(key);
    std::vector<TraceTransferBean>& transBeanList = it->second;

    TraceTransferBean traceData = TraceDataEncoder::encoderFromContextBean(&context);
    transBeanList.push_back(traceData);
  }

  auto it = transBeanMap.begin();

  while (it != transBeanMap.end()) {
    vector<string> key;
    UtilAll::Split(key, it->first, TraceConstants::CONTENT_SPLITOR);

    std::string dataTopic = it->first;
    std::string regionId = "";

    if (key.size() > 1) {
      dataTopic = key[0];
      regionId = key[1];
    }
    // flushData(entry.getValue(), dataTopic, regionId);
    flushData(it->second, dataTopic, regionId);

    it++;
  }
}

/**
 * Batch sending data actually
 */

void AsyncAppenderRequest::flushData(std::vector<TraceTransferBean> transBeanList,
                                     std::string dataTopic,
                                     std::string regionId) {
  if (transBeanList.size() == 0) {
    return;
  }
  // Temporary buffer
  string buffer;
  int count = 0;

  std::set<std::string> keySet;  //
  for (TraceTransferBean bean : transBeanList) {
    auto TK = bean.getTransKey();
    keySet.insert(TK.begin(), TK.end());

    buffer.append(bean.getTransData());
    count++;
    // Ensure that the size of the package should not exceed the upper limit.
    if (buffer.length() >= traceProducer->getMaxMessageSize()) {
      std::vector<std::string> keySetv;
      std::copy(keySet.begin(), keySet.end(), keySetv.begin());

      sendTraceDataByMQ(keySetv, buffer, dataTopic, regionId);
      // Clear temporary buffer after finishing

      // buffer.delete(0, buffer.length());

      keySet.clear();
      count = 0;
    }
  }
  if (count > 0) {
    // sendTraceDataByMQ(keySet, buffer.toString(), dataTopic, regionId);

    std::vector<std::string> keySetv;
    std::copy(keySet.begin(), keySet.end(), keySetv.begin());
    sendTraceDataByMQ(keySetv, buffer, dataTopic, regionId);
  }
  transBeanList.clear();
}

void AsyncAppenderRequest::sendTraceDataByMQ(std::vector<std::string> keySet,
                                             std::string data,
                                             std::string dataTopic,
                                             std::string regionId) {
  std::string traceTopic = traceTopicName;  //  "traceTopicName";
  if (AccessChannel::CLOUD == accessChannel) {
    traceTopic = TraceConstants::TRACE_TOPIC_PREFIX + regionId;
  }
  std::shared_ptr<MQMessage> message = std::shared_ptr<MQMessage>(new MQMessage(traceTopic, data.c_str()));
  // Keyset of message trace includes msgId of or original message
  message->setKeys(keySet);
  try {
    std::set<std::string> traceBrokerSet = tryGetMessageQueueBrokerSet(
        // traceProducer->getDefaultMQProducerImpl(),
        traceProducer, traceTopic);

    static TraceMessageSendCallback callback;
    // callback
    if (traceBrokerSet.empty()) {
      // No cross set
      traceProducer->send(*message, &callback, false);
    } else {
      static TraceMessageQueueSelector se;
      traceProducer->send(*message, &se, nullptr, &callback);
    }
    LOG_INFO("try send trace data,the traceData is %s", data.c_str());
  } catch (...) {
    LOG_INFO("catch send trace data,the traceData is %s", data.c_str());
  }
}

std::set<std::string> AsyncAppenderRequest::tryGetMessageQueueBrokerSet(DefaultMQProducer* producer,
                                                                        std::string topic) {
  MQClientFactory* clientFactory = producer->getFactory();

  std::set<std::string> brokerSet;
  auto topicPublishInfo = clientFactory->getTopicPublishInfoFromTable(topic);
  if (topicPublishInfo != nullptr) {
    boost::shared_ptr<TopicPublishInfo> info(new TopicPublishInfo());
    clientFactory->addTopicInfoToTable(topic, info);
    clientFactory->updateTopicRouteInfoFromNameServer(topic, producer->getSessionCredentials());
    topicPublishInfo = clientFactory->getTopicPublishInfoFromTable(topic);
  }
  return brokerSet;
}

}  // namespace rocketmq