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

#include "MQClientErrorContainer.h"

namespace rocketmq {
MQClientErrorContainer* MQClientErrorContainer::s_instance = nullptr;

MQClientErrorContainer* MQClientErrorContainer::instance() {
  if (!s_instance)
    s_instance = new MQClientErrorContainer();
  return s_instance;
}

void MQClientErrorContainer::setErr(std::string str) {
  boost::lock_guard<boost::mutex> lock(this->mutex);
  this->m_err = str;
}

std::string MQClientErrorContainer::getErr() {
  boost::lock_guard<boost::mutex> lock(this->mutex);
  return this->m_err;
}
}  // namespace rocketmq
