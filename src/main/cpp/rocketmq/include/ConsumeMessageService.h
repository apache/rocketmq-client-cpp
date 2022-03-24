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
#pragma once

#include <cstdint>

#include "ProcessQueue.h"
#include "rocketmq/MessageListener.h"

ROCKETMQ_NAMESPACE_BEGIN

class ConsumeMessageService {
public:
  virtual ~ConsumeMessageService() = default;

  /**
   * Start the dispatcher thread, which will dispatch messages in process queue to thread pool in form of runnable
   * functor.
   */
  virtual void start() = 0;

  /**
   * Stop the dispatcher thread and then reset the thread pool.
   */
  virtual void shutdown() = 0;

  virtual void submitConsumeTask(const std::weak_ptr<ProcessQueue>& process_queue) = 0;

  virtual void signalDispatcher() = 0;

  virtual void throttle(const std::string& topic, std::uint32_t threshold) = 0;
};

ROCKETMQ_NAMESPACE_END