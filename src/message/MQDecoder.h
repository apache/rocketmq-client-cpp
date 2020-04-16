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
#ifndef __MESSAGE_DECODER_H__
#define __MESSAGE_DECODER_H__

#include <map>
#include <memory>
#include <string>

#include "MQClientException.h"
#include "MQMessageExt.h"
#include "MQMessageId.h"
#include "MemoryInputStream.h"

namespace rocketmq {

class MQDecoder {
 public:
  static std::string createMessageId(const struct sockaddr* sa, int64_t offset);
  static MQMessageId decodeMessageId(const std::string& msgId);

  static MQMessageExtPtr2 decode(MemoryBlock& mem);
  static MQMessageExtPtr2 decode(MemoryBlock& mem, bool readBody);

  static std::vector<MQMessageExtPtr2> decodes(MemoryBlock& mem);
  static std::vector<MQMessageExtPtr2> decodes(MemoryBlock& mem, bool readBody);

  static std::string messageProperties2String(const std::map<std::string, std::string>& properties);
  static std::map<std::string, std::string> string2messageProperties(const std::string& properties);

  static std::string encodeMessage(MQMessage& message);
  static std::string encodeMessages(std::vector<MQMessagePtr>& msgs);

 private:
  static MQMessageExtPtr clientDecode(MemoryInputStream& byteBuffer, bool readBody);
  static MQMessageExtPtr decode(MemoryInputStream& byteBuffer, bool readBody);
  static MQMessageExtPtr decode(MemoryInputStream& byteBuffer, bool readBody, bool deCompressBody, bool isClient);

 public:
  static const char NAME_VALUE_SEPARATOR;
  static const char PROPERTY_SEPARATOR;
  static const int MSG_ID_LENGTH;
  static int MessageMagicCodePostion;
  static int MessageFlagPostion;
  static int MessagePhysicOffsetPostion;
  static int MessageStoreTimestampPostion;
};

}  // namespace rocketmq

#endif  // __MESSAGE_DECODER_H__
