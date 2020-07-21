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
#ifndef ROCKETMQ_COMMON_SERVICETHREAD_H_
#define ROCKETMQ_COMMON_SERVICETHREAD_H_

#include <atomic>
#include <memory>

#include "concurrent/latch.hpp"
#include "concurrent/thread.hpp"

namespace rocketmq {

class ServiceThread {
 public:
  ServiceThread()
      : wait_point_(1), has_notified_(false), stopped_(false), is_daemon_(false), thread_(nullptr), started_(false) {}
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
  latch wait_point_;
  std::atomic<bool> has_notified_;
  volatile bool stopped_;
  bool is_daemon_;

 private:
  std::unique_ptr<thread> thread_;
  std::atomic<bool> started_;
};

}  // namespace rocketmq

#endif  // ROCKETMQ_COMMON_SERVICETHREAD_H_
