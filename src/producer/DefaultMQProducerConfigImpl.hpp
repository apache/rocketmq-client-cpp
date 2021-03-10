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
#ifndef ROCKETMQ_PRODUCER_DEFAULTMQPRODUCERCONFIGIMPL_HPP_
#define ROCKETMQ_PRODUCER_DEFAULTMQPRODUCERCONFIGIMPL_HPP_

#include <algorithm>  // std::min, std::max
#include <thread>

#include "DefaultMQProducerConfig.h"
#include "MQClientConfigImpl.hpp"

namespace rocketmq {

/**
 * DefaultMQProducerConfigImpl - implement for DefaultMQProducerConfig
 */
class DefaultMQProducerConfigImpl : virtual public DefaultMQProducerConfig, public MQClientConfigImpl {
 public:
  DefaultMQProducerConfigImpl()
      : async_send_thread_nums_(std::min(4, (int)std::thread::hardware_concurrency())),
        max_message_size_(1024 * 1024 * 4),         // 4MB
        compress_msg_body_over_howmuch_(1024 * 4),  // 4KB
        compress_level_(5),
        send_msg_timeout_(3000),
        retry_times_(2),
        retry_times_for_async_(2),
        retry_another_broker_when_not_store_ok_(false) {}

  virtual ~DefaultMQProducerConfigImpl() = default;

  int async_send_thread_nums() const override { return async_send_thread_nums_; }
  void set_async_send_thread_nums(int async_send_thread_nums) override {
    async_send_thread_nums_ = async_send_thread_nums;
  }

  int max_message_size() const override { return max_message_size_; }
  void set_max_message_size(int max_message_size) override { max_message_size_ = max_message_size; }

  int compress_msg_body_over_howmuch() const override { return compress_msg_body_over_howmuch_; }
  void set_compress_msg_body_over_howmuch(int compress_msg_body_over_howmuch) override {
    compress_msg_body_over_howmuch_ = compress_msg_body_over_howmuch;
  }

  int compress_level() const override { return compress_level_; }
  void set_compress_level(int compress_level) override {
    if ((compress_level >= 0 && compress_level <= 9) || compress_level == -1) {
      compress_level_ = compress_level;
    }
  }

  int send_msg_timeout() const override { return send_msg_timeout_; }
  void set_send_msg_timeout(int send_msg_timeout) override { send_msg_timeout_ = send_msg_timeout; }

  int retry_times() const override { return retry_times_; }
  void set_retry_times(int retry_times) override { retry_times_ = std::min(std::max(0, retry_times), 15); }

  int retry_times_for_async() const override { return retry_times_for_async_; }
  void set_retry_times_for_async(int retry_times) override {
    retry_times_for_async_ = std::min(std::max(0, retry_times), 15);
  }

  bool retry_another_broker_when_not_store_ok() const override { return retry_another_broker_when_not_store_ok_; }
  void set_retry_another_broker_when_not_store_ok(bool retry_another_broker_when_not_store_ok) override {
    retry_another_broker_when_not_store_ok_ = retry_another_broker_when_not_store_ok;
  }

 protected:
  int async_send_thread_nums_;
  int max_message_size_;                // default: 4 MB
  int compress_msg_body_over_howmuch_;  // default: 4 KB
  int compress_level_;
  int send_msg_timeout_;
  int retry_times_;
  int retry_times_for_async_;
  bool retry_another_broker_when_not_store_ok_;
};

}  // namespace rocketmq

#endif  // ROCKETMQ_PRODUCER_DEFAULTMQPRODUCERCONFIGIMPL_HPP_
