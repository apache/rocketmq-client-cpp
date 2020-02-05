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
#include "include/BatchMessage.h"

#include "MQDecoder.h"
#include "StringIdMaker.h"

namespace rocketmq {

std::string BatchMessage::encode(std::vector<MQMessage>& msgs) {
  std::string encodedBody;
  for (auto message : msgs) {
    std::string unique_id = StringIdMaker::getInstance().createUniqID();
    message.setProperty(MQMessage::PROPERTY_UNIQ_CLIENT_MESSAGE_ID_KEYIDX, unique_id);
    encodedBody.append(encode(message));
  }
  return encodedBody;
}

std::string BatchMessage::encode(MQMessage& message) {
  string encodeMsg;
  const string& body = message.getBody();
  int bodyLen = body.length();
  string properties = MQDecoder::messageProperties2String(message.getProperties());
  short propertiesLength = (short)properties.length();
  int storeSize = 20 + bodyLen + 2 + propertiesLength;
  // TOTALSIZE|MAGICCOD|BODYCRC|FLAG|BODYLen|Body|propertiesLength|properties
  int magicCode = 0;
  int bodyCrc = 0;
  int flag = message.getFlag();
  int storeSize_net = htonl(storeSize);
  int magicCode_net = htonl(magicCode);
  int bodyCrc_net = htonl(bodyCrc);
  int flag_net = htonl(flag);
  int bodyLen_net = htonl(bodyLen);
  int propertiesLength_net = htons(propertiesLength);
  encodeMsg.append((char*)&storeSize_net, sizeof(int));
  encodeMsg.append((char*)&magicCode_net, sizeof(int));
  encodeMsg.append((char*)&bodyCrc_net, sizeof(int));
  encodeMsg.append((char*)&flag_net, sizeof(int));
  encodeMsg.append((char*)&bodyLen_net, sizeof(int));
  encodeMsg.append(body.c_str(), body.length());
  encodeMsg.append((char*)&propertiesLength_net, sizeof(short));
  encodeMsg.append(properties.c_str(), propertiesLength);
  return encodeMsg;
}

}  // namespace rocketmq
