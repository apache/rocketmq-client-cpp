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
#include "ServiceThread.h"

#include "Logging.h"
#include "UtilAll.h"

namespace rocketmq {

void ServiceThread::start() {
  LOG_INFO_NEW("Try to start service thread:{} started:{} lastThread:{}", getServiceName(), m_started.load(),
               (void*)m_thread.get());
  bool expected = false;
  if (!m_started.compare_exchange_strong(expected, true)) {
    return;
  }
  m_stopped = false;
  m_thread.reset(new thread(getServiceName(), &ServiceThread::run, this));
  m_thread->start();
}

void ServiceThread::shutdown() {
  LOG_INFO_NEW("Try to shutdown service thread:{} started:{} lastThread:{}", getServiceName(), m_started.load(),
               (void*)m_thread.get());
  bool expected = true;
  if (!m_started.compare_exchange_strong(expected, false)) {
    return;
  }
  m_stopped = true;
  LOG_INFO_NEW("shutdown thread {}", getServiceName());

  wakeup();

  int64_t beginTime = UtilAll::currentTimeMillis();
  m_thread->join();
  int64_t elapsedTime = UtilAll::currentTimeMillis() - beginTime;
  LOG_INFO_NEW("join thread {} elapsed time(ms) {}", getServiceName(), elapsedTime);
}

void ServiceThread::wakeup() {
  bool expected = false;
  if (m_hasNotified.compare_exchange_strong(expected, true)) {
    m_waitPoint.count_down();  // notify
  }
}

void ServiceThread::waitForRunning(long interval) {
  bool expected = true;
  if (m_hasNotified.compare_exchange_strong(expected, false)) {
    onWaitEnd();
    return;
  }

  // entry to wait
  m_waitPoint.reset();

  try {
    m_waitPoint.wait(interval, time_unit::milliseconds);
  } catch (const std::exception& e) {
    LOG_WARN_NEW("encounter unexpected exception: {}", e.what());
  }
  m_hasNotified.store(false);
  onWaitEnd();
}

void ServiceThread::onWaitEnd() {}

bool ServiceThread::isStopped() {
  return m_stopped;
};

}  // namespace rocketmq
