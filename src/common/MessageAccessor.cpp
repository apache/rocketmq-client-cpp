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
#include "MessageAccessor.h"
#include <functional>
#include <vector>
#include "Logging.h"
#include "NameSpaceUtil.h"

using namespace std;
namespace rocketmq {

void MessageAccessor::withNameSpace(MQMessage& msg, const string& nameSpace) {
  if (!nameSpace.empty()) {
    string originTopic = msg.getTopic();
    string newTopic = nameSpace + NAMESPACE_SPLIT_FLAG + originTopic;
    msg.setTopic(newTopic);
  }
}

void MessageAccessor::withoutNameSpaceSingle(MQMessageExt& msg, const string& nameSpace) {
  if (!nameSpace.empty()) {
    string originTopic = msg.getTopic();
    auto index = originTopic.find(nameSpace);
    if (index != string::npos) {
      string newTopic =
          originTopic.substr(index + nameSpace.length() + NAMESPACE_SPLIT_FLAG.length(), originTopic.length());
      msg.setTopic(newTopic);
      LOG_DEBUG("Find Name Space Prefix in MessageID[%s], OriginTopic[%s], NewTopic[%s]", msg.getMsgId().c_str(),
                originTopic.c_str(), newTopic.c_str());
    }
  }
}
void MessageAccessor::withoutNameSpace(vector<MQMessageExt>& msgs, const string& nameSpace) {
  if (!nameSpace.empty()) {
    // for_each(msgs.cbegin(), msgs.cend(), bind2nd(&MessageAccessor::withoutNameSpaceSingle, nameSpace));
    for (auto iter = msgs.begin(); iter != msgs.end(); iter++) {
      withoutNameSpaceSingle(*iter, nameSpace);
    }
  }
}
//<!***************************************************************************
}  // namespace rocketmq
