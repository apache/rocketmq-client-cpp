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
  LOG_INFO_NEW("Try to start service thread:{} started:{} lastThread:{}", getServiceName(), started_.load(),
               (void*)thread_.get());
  bool expected = false;
  if (!started_.compare_exchange_strong(expected, true)) {
    return;
  }
  stopped_ = false;
  thread_.reset(new thread(getServiceName(), &ServiceThread::run, this));
  thread_->start();
}

void ServiceThread::shutdown() {
  LOG_INFO_NEW("Try to shutdown service thread:{} started:{} lastThread:{}", getServiceName(), started_.load(),
               (void*)thread_.get());
  bool expected = true;
  if (!started_.compare_exchange_strong(expected, false)) {
    return;
  }
  stopped_ = true;
  LOG_INFO_NEW("shutdown thread {}", getServiceName());

  wakeup();

  int64_t beginTime = UtilAll::currentTimeMillis();
  thread_->join();
  int64_t elapsedTime = UtilAll::currentTimeMillis() - beginTime;
  LOG_INFO_NEW("join thread {} elapsed time(ms) {}", getServiceName(), elapsedTime);
}

void ServiceThread::wakeup() {
  bool expected = false;
  if (has_notified_.compare_exchange_strong(expected, true)) {
    wait_point_.count_down();  // notify
  }
}

void ServiceThread::waitForRunning(long interval) {
  bool expected = true;
  if (has_notified_.compare_exchange_strong(expected, false)) {
    onWaitEnd();
    return;
  }

  // entry to wait
  wait_point_.reset();

  try {
    wait_point_.wait(interval, time_unit::milliseconds);
  } catch (const std::exception& e) {
    LOG_WARN_NEW("encounter unexpected exception: {}", e.what());
  }
  has_notified_.store(false);
  onWaitEnd();
}

void ServiceThread::onWaitEnd() {}

bool ServiceThread::isStopped() {
  return stopped_;
};

}  // namespace rocketmq
