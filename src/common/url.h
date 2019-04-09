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
#ifndef ROCKETMQ_CLIENT4CPP_URL_HH_
#define ROCKETMQ_CLIENT4CPP_URL_HH_

#include <string>

namespace rocketmq {
class Url {
 public:
  Url(const std::string& url_s);  // omitted copy, ==, accessors, ...

 private:
  void parse(const std::string& url_s);

 public:
  std::string protocol_;
  std::string host_;
  std::string port_;
  std::string path_;
  std::string query_;
};
}
#endif  // ROCKETMQ_CLIENT4CPP_URL_HH_
