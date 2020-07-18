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
#include "MessageDecoder.h"

#include <cstring>  // std::memcpy

#include <algorithm>  // std::move
#include <sstream>    // std::stringstream

#ifndef WIN32
#include <arpa/inet.h>   // htons, htonl
#include <netinet/in.h>  // struct sockaddr, sockaddr_in, sockaddr_in6
#endif

#include "ByteOrder.h"
#include "Logging.h"
#include "MemoryOutputStream.h"
#include "MessageExtImpl.h"
#include "MessageAccessor.h"
#include "MessageSysFlag.h"
#include "UtilAll.h"

namespace rocketmq {

const int MessageDecoder::MSG_ID_LENGTH = 8 + 8;

const char MessageDecoder::NAME_VALUE_SEPARATOR = 1;
const char MessageDecoder::PROPERTY_SEPARATOR = 2;

int MessageDecoder::MessageMagicCodePostion = 4;
int MessageDecoder::MessageFlagPostion = 16;
int MessageDecoder::MessagePhysicOffsetPostion = 28;
int MessageDecoder::MessageStoreTimestampPostion = 56;

std::string MessageDecoder::createMessageId(const struct sockaddr* sa, int64_t offset) {
  MemoryOutputStream mos(sa->sa_family == AF_INET ? 16 : 28);
  if (sa->sa_family == AF_INET) {
    const struct sockaddr_in* sin = (struct sockaddr_in*)sa;
    mos.write(&sin->sin_addr.s_addr, 4);
    mos.writeRepeatedByte(0, 2);
    mos.write(&sin->sin_port, 2);
    mos.writeInt64BigEndian(offset);
  } else {
    const struct sockaddr_in6* sin6 = (struct sockaddr_in6*)sa;
    mos.write(&sin6->sin6_addr, 16);
    mos.writeRepeatedByte(0, 2);
    mos.write(&sin6->sin6_port, 2);
    mos.writeInt64BigEndian(offset);
  }

  const char* bytes = static_cast<const char*>(mos.getData());
  int len = mos.getDataSize();

  return UtilAll::bytes2string(bytes, len);
}

MessageId MessageDecoder::decodeMessageId(const std::string& msgId) {
  size_t ip_length = msgId.length() == 32 ? 4 * 2 : 16 * 2;

  std::string ip = msgId.substr(0, ip_length);
  std::string port = msgId.substr(ip_length, 8);
  std::string offset = msgId.substr(ip_length + 8);

  uint16_t portInt = (uint16_t)std::stoul(port, nullptr, 16);
  uint64_t offsetInt = std::stoull(offset, nullptr, 16);

  if (ip_length == 32) {
    int32_t ipInt = std::stoul(ip, nullptr, 16);
    struct sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_port = htons(portInt);
    sin.sin_addr.s_addr = htonl(ipInt);
    return MessageId((struct sockaddr*)&sin, offsetInt);
  } else {
    struct sockaddr_in6 sin6;
    sin6.sin6_family = AF_INET6;
    sin6.sin6_port = htons(portInt);
    UtilAll::string2bytes((char*)&sin6.sin6_addr, ip);
    return MessageId((struct sockaddr*)&sin6, offsetInt);
  }
}

MessageExtPtr MessageDecoder::clientDecode(MemoryInputStream& byteBuffer, bool readBody) {
  return decode(byteBuffer, readBody, true, true);
}

MessageExtPtr MessageDecoder::decode(MemoryInputStream& byteBuffer, bool readBody) {
  return decode(byteBuffer, readBody, true, false);
}

MessageExtPtr MessageDecoder::decode(MemoryInputStream& byteBuffer, bool readBody, bool deCompressBody, bool isClient) {
  auto msgExt = isClient ? std::make_shared<MessageClientExtImpl>() : std::make_shared<MessageExtImpl>();

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
  msgExt->setBornHost(ipPort2SocketAddress(bornHost, bornPort));

  // 11 STORETIMESTAMP
  int64_t storeTimestamp = byteBuffer.readInt64BigEndian();
  msgExt->setStoreTimestamp(storeTimestamp);

  // 12 STOREHOST
  int32_t storeHost = byteBuffer.readIntBigEndian();
  int32_t storePort = byteBuffer.readIntBigEndian();
  msgExt->setStoreHost(ipPort2SocketAddress(storeHost, storePort));

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
  msgExt->MessageExtImpl::setMsgId(msgId);

  if (uncompress_failed) {
    LOG_WARN_NEW("can not uncompress message, id:{}", msgExt->getMsgId());
  }

  return msgExt;
}

MessageExtPtr MessageDecoder::decode(MemoryBlock& mem) {
  return decode(mem, true);
}

MessageExtPtr MessageDecoder::decode(MemoryBlock& mem, bool readBody) {
  MemoryInputStream rawInput(mem, true);
  return decode(rawInput, readBody);
}

std::vector<MessageExtPtr> MessageDecoder::decodes(MemoryBlock& mem) {
  return decodes(mem, true);
}

std::vector<MessageExtPtr> MessageDecoder::decodes(MemoryBlock& mem, bool readBody) {
  std::vector<MessageExtPtr> msgExts;
  MemoryInputStream rawInput(mem, true);
  while (rawInput.getNumBytesRemaining() > 0) {
    auto msgExt = clientDecode(rawInput, readBody);
    if (nullptr == msgExt) {
      break;
    }
    msgExts.emplace_back(msgExt);
  }
  return msgExts;
}

std::string MessageDecoder::messageProperties2String(const std::map<std::string, std::string>& properties) {
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

std::map<std::string, std::string> MessageDecoder::string2messageProperties(const std::string& properties) {
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

std::string MessageDecoder::encodeMessage(Message& message) {
  const std::string& body = message.getBody();
  uint32_t bodyLen = body.length();
  std::string properties = MessageDecoder::messageProperties2String(message.getProperties());
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

std::string MessageDecoder::encodeMessages(std::vector<MQMessage>& msgs) {
  std::string encodedBody;
  for (auto msg : msgs) {
    encodedBody.append(encodeMessage(msg));
  }
  return encodedBody;
}

}  // namespace rocketmq
