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
#include "MQDecoder.h"

#include <sstream>

#ifndef WIN32
#include <netinet/in.h>
#endif

#include "ByteOrder.h"
#include "Logging.h"
#include "MemoryOutputStream.h"
#include "MessageAccessor.h"
#include "MessageSysFlag.h"
#include "UtilAll.h"

namespace rocketmq {

const int MQDecoder::MSG_ID_LENGTH = 8 + 8;

const char MQDecoder::NAME_VALUE_SEPARATOR = 1;
const char MQDecoder::PROPERTY_SEPARATOR = 2;

int MQDecoder::MessageMagicCodePostion = 4;
int MQDecoder::MessageFlagPostion = 16;
int MQDecoder::MessagePhysicOffsetPostion = 28;
int MQDecoder::MessageStoreTimestampPostion = 56;

std::string MQDecoder::createMessageId(sockaddr addr, int64_t offset) {
  struct sockaddr_in* sa = (struct sockaddr_in*)&addr;

  MemoryOutputStream outputmen(MSG_ID_LENGTH);
  outputmen.writeIntBigEndian(sa->sin_addr.s_addr);
  outputmen.writeRepeatedByte(0, 2);
  outputmen.write(&(sa->sin_port), 2);
  outputmen.writeInt64BigEndian(offset);

  const char* bytes = static_cast<const char*>(outputmen.getData());
  int len = outputmen.getDataSize();

  return UtilAll::bytes2string(bytes, len);
}

MQMessageId MQDecoder::decodeMessageId(const std::string& msgId) {
  std::string ipStr = msgId.substr(0, 8);
  std::string portStr = msgId.substr(8, 8);
  std::string offsetStr = msgId.substr(16);

  size_t pos;
  int ipInt = std::stoul(ipStr, &pos, 16);
  int portInt = std::stoul(portStr, &pos, 16);

  uint64_t offset = UtilAll::hexstr2ull(offsetStr.c_str());

  struct sockaddr_in sa;
  sa.sin_family = AF_INET;
  sa.sin_port = htons(portInt);
  sa.sin_addr.s_addr = htonl(ipInt);

  return MQMessageId(*((sockaddr*)&sa), offset);
}

MQMessageExtPtr MQDecoder::clientDecode(MemoryInputStream& byteBuffer, bool readBody) {
  return decode(byteBuffer, readBody, true, true);
}

MQMessageExtPtr MQDecoder::decode(MemoryInputStream& byteBuffer, bool readBody) {
  return decode(byteBuffer, readBody, true, false);
}

MQMessageExtPtr MQDecoder::decode(MemoryInputStream& byteBuffer, bool readBody, bool deCompressBody, bool isClient) {
  MQMessageExtPtr msgExt;

  if (isClient) {
    msgExt = new MQMessageClientExt();
  } else {
    msgExt = new MQMessageExt();
  }

  // 1 TOTALSIZE
  int32_t storeSize = byteBuffer.readIntBigEndian();
  msgExt->setStoreSize(storeSize);

  // 2 MAGICCODE sizeof(int)
  byteBuffer.skipNextBytes(sizeof(int));

  // 3 BODYCRC
  int32_t bodyCRC = byteBuffer.readIntBigEndian();
  msgExt->setBodyCRC(bodyCRC);

  // 4 QUEUEID
  int32_t queueId = byteBuffer.readIntBigEndian();
  msgExt->setQueueId(queueId);

  // 5 FLAG
  int32_t flag = byteBuffer.readIntBigEndian();
  msgExt->setFlag(flag);

  // 6 QUEUEOFFSET
  int64_t queueOffset = byteBuffer.readInt64BigEndian();
  msgExt->setQueueOffset(queueOffset);

  // 7 PHYSICALOFFSET
  int64_t physicOffset = byteBuffer.readInt64BigEndian();
  msgExt->setCommitLogOffset(physicOffset);

  // 8 SYSFLAG
  int32_t sysFlag = byteBuffer.readIntBigEndian();
  msgExt->setSysFlag(sysFlag);

  // 9 BORNTIMESTAMP
  int64_t bornTimeStamp = byteBuffer.readInt64BigEndian();
  msgExt->setBornTimestamp(bornTimeStamp);

  // 10 BORNHOST
  int32_t bornHost = byteBuffer.readIntBigEndian();
  int32_t bornPort = byteBuffer.readIntBigEndian();
  msgExt->setBornHost(IPPort2socketAddress(bornHost, bornPort));

  // 11 STORETIMESTAMP
  int64_t storeTimestamp = byteBuffer.readInt64BigEndian();
  msgExt->setStoreTimestamp(storeTimestamp);

  // 12 STOREHOST
  int32_t storeHost = byteBuffer.readIntBigEndian();
  int32_t storePort = byteBuffer.readIntBigEndian();
  msgExt->setStoreHost(IPPort2socketAddress(storeHost, storePort));

  // 13 RECONSUMETIMES
  int32_t reconsumeTimes = byteBuffer.readIntBigEndian();
  msgExt->setReconsumeTimes(reconsumeTimes);

  // 14 Prepared Transaction Offset
  int64_t preparedTransactionOffset = byteBuffer.readInt64BigEndian();
  msgExt->setPreparedTransactionOffset(preparedTransactionOffset);

  // 15 BODY
  int uncompress_failed = false;
  int32_t bodyLen = byteBuffer.readIntBigEndian();
  if (bodyLen > 0) {
    if (readBody) {
      MemoryPool block;
      byteBuffer.readIntoMemoryBlock(block, bodyLen);

      // decompress body
      if (deCompressBody && (sysFlag & MessageSysFlag::CompressedFlag) == MessageSysFlag::CompressedFlag) {
        std::string outbody;
        if (UtilAll::inflate(block.getData(), block.getSize(), outbody)) {
          msgExt->setBody(std::move(outbody));
        } else {
          uncompress_failed = true;
        }
      } else {
        msgExt->setBody(block.getData(), block.getSize());
      }
    } else {
      byteBuffer.skipNextBytes(bodyLen);
    }
  }

  // 16 TOPIC
  int8_t topicLen = byteBuffer.readByte();
  MemoryPool block;
  byteBuffer.readIntoMemoryBlock(block, topicLen);
  const char* const pTopic = static_cast<const char*>(block.getData());
  topicLen = block.getSize();
  msgExt->setTopic(pTopic, topicLen);

  // 17 properties
  int16_t propertiesLen = byteBuffer.readShortBigEndian();
  if (propertiesLen > 0) {
    MemoryPool block;
    byteBuffer.readIntoMemoryBlock(block, propertiesLen);
    std::string propertiesString(block.getData(), block.getSize());

    std::map<std::string, std::string> propertiesMap = string2messageProperties(propertiesString);
    MessageAccessor::setProperties(*msgExt, std::move(propertiesMap));
  }

  // 18 msg ID
  std::string msgId = createMessageId(msgExt->getStoreHost(), (int64_t)msgExt->getCommitLogOffset());
  msgExt->MQMessageExt::setMsgId(msgId);

  if (uncompress_failed) {
    LOG_WARN_NEW("can not uncompress message, id:{}", msgExt->getMsgId());
  }

  return msgExt;
}

MQMessageExtPtr2 MQDecoder::decode(MemoryBlock& mem) {
  return decode(mem, true);
}

MQMessageExtPtr2 MQDecoder::decode(MemoryBlock& mem, bool readBody) {
  MemoryInputStream rawInput(mem, true);
  MQMessageExtPtr2 message(decode(rawInput, readBody));
  return message;
}

std::vector<MQMessageExtPtr2> MQDecoder::decodes(MemoryBlock& mem) {
  return decodes(mem, true);
}

std::vector<MQMessageExtPtr2> MQDecoder::decodes(MemoryBlock& mem, bool readBody) {
  std::vector<MQMessageExtPtr2> msgExts;
  MemoryInputStream rawInput(mem, true);
  while (rawInput.getNumBytesRemaining() > 0) {
    auto* msgExt = clientDecode(rawInput, readBody);
    if (msgExt != nullptr) {
      msgExts.emplace_back(msgExt);
    } else {
      break;
    }
  }
  return msgExts;
}

std::string MQDecoder::messageProperties2String(const std::map<std::string, std::string>& properties) {
  std::string os;

  for (const auto& it : properties) {
    const auto& name = it.first;
    const auto& value = it.second;

    // skip compressed flag
    if (MQMessageConst::PROPERTY_ALREADY_COMPRESSED_FLAG == name) {
      continue;
    }

    os.append(name);
    os += NAME_VALUE_SEPARATOR;
    os.append(value);
    os += PROPERTY_SEPARATOR;
  }

  return os;
}

std::map<std::string, std::string> MQDecoder::string2messageProperties(const std::string& properties) {
  std::vector<std::string> out;
  UtilAll::Split(out, properties, PROPERTY_SEPARATOR);

  std::map<std::string, std::string> map;
  for (size_t i = 0; i < out.size(); i++) {
    std::vector<std::string> outValue;
    UtilAll::Split(outValue, out[i], NAME_VALUE_SEPARATOR);

    if (outValue.size() == 2) {
      map[outValue[0]] = outValue[1];
    }
  }
  return map;
}

std::string MQDecoder::encodeMessage(MQMessage& message) {
  const std::string& body = message.getBody();
  uint32_t bodyLen = body.length();
  std::string properties = MQDecoder::messageProperties2String(message.getProperties());
  uint16_t propertiesLength = (int16_t)properties.length();
  uint32_t storeSize = 4                        // 1 TOTALSIZE
                       + 4                      // 2 MAGICCODE
                       + 4                      // 3 BODYCRC
                       + 4                      // 4 FLAG
                       + 4 + bodyLen            // 5 BODY
                       + 2 + propertiesLength;  // 6 PROPERTIES

  // TOTALSIZE|MAGICCODE|BODYCRC|FLAG|BodyLen|Body|propertiesLength|properties
  std::string encodeMsg;

  // 1 TOTALSIZE
  uint32_t storeSize_net = ByteOrder::swapIfLittleEndian(storeSize);
  encodeMsg.append((char*)&storeSize_net, sizeof(uint32_t));

  // 2 MAGICCODE
  uint32_t magicCode = 0;
  uint32_t magicCode_net = ByteOrder::swapIfLittleEndian(magicCode);
  encodeMsg.append((char*)&magicCode_net, sizeof(uint32_t));

  // 3 BODYCRC
  uint32_t bodyCrc = 0;
  uint32_t bodyCrc_net = ByteOrder::swapIfLittleEndian(bodyCrc);
  encodeMsg.append((char*)&bodyCrc_net, sizeof(uint32_t));

  // 4 FLAG
  uint32_t flag = message.getFlag();
  uint32_t flag_net = ByteOrder::swapIfLittleEndian(flag);
  encodeMsg.append((char*)&flag_net, sizeof(uint32_t));

  // 5 BODY
  uint32_t bodyLen_net = ByteOrder::swapIfLittleEndian(bodyLen);
  encodeMsg.append((char*)&bodyLen_net, sizeof(uint32_t));
  encodeMsg.append(body);

  // 6 properties
  uint16_t propertiesLength_net = ByteOrder::swapIfLittleEndian(propertiesLength);
  encodeMsg.append((char*)&propertiesLength_net, sizeof(uint16_t));
  encodeMsg.append(std::move(properties));

  return encodeMsg;
}

std::string MQDecoder::encodeMessages(std::vector<MQMessagePtr>& msgs) {
  std::string encodedBody;
  for (auto* message : msgs) {
    encodedBody.append(encodeMessage(*message));
  }
  return encodedBody;
}

}  // namespace rocketmq
