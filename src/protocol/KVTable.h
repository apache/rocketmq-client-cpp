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
#ifndef __KV_TABLE_H__
#define __KV_TABLE_H__

#include <map>
#include <string>

#include "RemotingSerializable.h"

namespace rocketmq {

class KVTable : public RemotingSerializable {
 public:
  virtual ~KVTable() { m_table.clear(); }

  const std::map<std::string, std::string>& getTable() { return m_table; }

  void setTable(const std::map<std::string, std::string>& table) { m_table = table; }

 private:
  std::map<std::string, std::string> m_table;
};

}  // namespace rocketmq

#endif  // __KV_TABLE_H__
