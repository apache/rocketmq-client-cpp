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

#ifndef __NAMESPACEUTIL_H__
#define __NAMESPACEUTIL_H__

#include <string>

using namespace std;

static const string ENDPOINT_PREFIX = "http://";
static const unsigned int ENDPOINT_PREFIX_LENGTH = ENDPOINT_PREFIX.length();
static const string NAMESPACE_PREFIX = "MQ_INST_";
static const int NAMESPACE_PREFIX_LENGTH = NAMESPACE_PREFIX.length();
static const string NAMESPACE_SPLIT_FLAG = "%";

namespace rocketmq {
class NameSpaceUtil {
 public:
  static bool isEndPointURL(const string& nameServerAddr);

  static string formatNameServerURL(const string& nameServerAddr);

  static string getNameSpaceFromNsURL(const string& nameServerAddr);

  static bool checkNameSpaceExistInNsURL(const string& nameServerAddr);

  static bool checkNameSpaceExistInNameServer(const string& nameServerAddr);

  static string withNameSpace(const string& source, const string& ns);

  static string withoutNameSpace(const string& source, const string& ns);

  static bool hasNameSpace(const string& source, const string& ns);
};

}  // namespace rocketmq
#endif  //__NAMESPACEUTIL_H__
