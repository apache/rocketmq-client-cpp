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
#ifndef ROCKETMQ_CONSUMER_FINDBROKERRESULT_HPP_
#define ROCKETMQ_CONSUMER_FINDBROKERRESULT_HPP_

#include <string>  // std::string

namespace rocketmq {

class FindBrokerResult {
 public:
  FindBrokerResult(const std::string& sbrokerAddr, bool bslave) : broker_addr_(sbrokerAddr), slave_(bslave) {}

  inline const std::string& broker_addr() const { return broker_addr_; }
  inline void set_borker_addr(const std::string& broker_addr) { broker_addr_ = broker_addr; }

  inline bool slave() const { return slave_; }
  inline void set_slave(bool slave) { slave_ = slave; }

 private:
  std::string broker_addr_;
  bool slave_;
};

}  // namespace rocketmq

#endif  // ROCKETMQ_CONSUMER_FINDBROKERRESULT_HPP_
