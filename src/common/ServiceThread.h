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
#ifndef __SERVICE_THREAD_H__
#define __SERVICE_THREAD_H__

#include <atomic>
#include <memory>

#include "concurrent/latch.hpp"
#include "concurrent/thread.hpp"

namespace rocketmq {

class ServiceThread {
 public:
  ServiceThread()
      : m_waitPoint(1),
        m_hasNotified(false),
        m_stopped(false),
        m_isDaemon(false),
        m_thread(nullptr),
        m_started(false) {}
  virtual ~ServiceThread() = default;

  virtual std::string getServiceName() = 0;
  virtual void run() = 0;

  virtual void start();
  virtual void shutdown();

  void wakeup();

  bool isStopped();

 protected:
  void waitForRunning(long interval);
  virtual void onWaitEnd();

 protected:
  latch m_waitPoint;
  std::atomic<bool> m_hasNotified;
  volatile bool m_stopped;
  bool m_isDaemon;

 private:
  std::unique_ptr<thread> m_thread;
  std::atomic<bool> m_started;
};

}  // namespace rocketmq

#endif  // __PULL_MESSAGE_SERVICE_H__
