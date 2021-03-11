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

#include <algorithm>  // std::move
#include <sstream>    // std::stringstream

#ifndef WIN32
#include <arpa/inet.h>   // htons
#include <netinet/in.h>  // sockaddr_in, sockaddr_in6
#else
#include "Winsock2.h"
#endif

#include "ByteOrder.h"
#include "Logging.h"
#include "MessageAccessor.hpp"
#include "MessageExtImpl.h"
#include "MessageSysFlag.h"
#include "SocketUtil.h"
#include "UtilAll.h"

static const char NAME_VALUE_SEPARATOR = 1;
static const char PROPERTY_SEPARATOR = 2;

namespace rocketmq {

std::string MessageDecoder::createMessageId(const struct sockaddr* sa, int64_t offset) {
  int msgIdLength = IpaddrSize(sa) + /* port field size */ 4 + sizeof(offset);
  std::unique_ptr<ByteBuffer> byteBuffer(ByteBuffer::allocate(msgIdLength));
  if (sa->sa_family == AF_INET) {
    struct sockaddr_in* sin = (struct sockaddr_in*)sa;
    byteBuffer->put(ByteArray(reinterpret_cast<char*>(&sin->sin_addr), kIPv4AddrSize));
    byteBuffer->putInt(ntohs(sin->sin_port));
  } else {
    struct sockaddr_in6* sin6 = (struct sockaddr_in6*)sa;
    byteBuffer->put(ByteArray(reinterpret_cast<char*>(&sin6->sin6_addr), kIPv6AddrSize));
    byteBuffer->putInt(ntohs(sin6->sin6_port));
  }
  byteBuffer->putLong(offset);
  byteBuffer->flip();
  return UtilAll::bytes2string(byteBuffer->array(), msgIdLength);
}

MessageId MessageDecoder::decodeMessageId(const std::string& msgId) {
  size_t ip_length = msgId.length() == 32 ? kIPv4AddrSize * 2 : kIPv6AddrSize * 2;

  ByteArray byteArray(ip_length / 2);
  std::string ip = msgId.substr(0, ip_length);
  UtilAll::string2bytes(byteArray.array(), ip);

  std::string port = msgId.substr(ip_length, 8);
  uint32_t portInt = std::stoul(port, nullptr, 16);

  auto* sin = IPPortToSockaddr(byteArray, portInt);

  std::string offset = msgId.substr(ip_length + 8);
  uint64_t offsetInt = std::stoull(offset, nullptr, 16);

  return MessageId(sin, offsetInt);
}

MessageExtPtr MessageDecoder::clientDecode(ByteBuffer& byteBuffer, bool readBody) {
  return decode(byteBuffer, readBody, true, true);
}

MessageExtPtr MessageDecoder::decode(ByteBuffer& byteBuffer) {
  return decode(byteBuffer, true);
}

MessageExtPtr MessageDecoder::decode(ByteBuffer& byteBuffer, bool readBody) {
  return decode(byteBuffer, readBody, true, false);
}

MessageExtPtr MessageDecoder::decode(ByteBuffer& byteBuffer, bool readBody, bool deCompressBody, bool isClient) {
  auto msgExt = isClient ? std::make_shared<MessageClientExtImpl>() : std::make_shared<MessageExtImpl>();

  // 1 TOTALSIZE
  int32_t storeSize = byteBuffer.getInt();
  msgExt->set_store_size(storeSize);

  // 2 MAGICCODE sizeof(int)
  byteBuffer.getInt();

  // 3 BODYCRC
  int32_t bodyCRC = byteBuffer.getInt();
  msgExt->set_body_crc(bodyCRC);

  // 4 QUEUEID
  int32_t queueId = byteBuffer.getInt();
  msgExt->set_queue_id(queueId);

  // 5 FLAG
  int32_t flag = byteBuffer.getInt();
  msgExt->set_flag(flag);

  // 6 QUEUEOFFSET
  int64_t queueOffset = byteBuffer.getLong();
  msgExt->set_queue_offset(queueOffset);

  // 7 PHYSICALOFFSET
  int64_t physicOffset = byteBuffer.getLong();
  msgExt->set_commit_log_offset(physicOffset);

  // 8 SYSFLAG
  int32_t sysFlag = byteBuffer.getInt();
  msgExt->set_sys_flag(sysFlag);

  // 9 BORNTIMESTAMP
  int64_t bornTimeStamp = byteBuffer.getLong();
  msgExt->set_born_timestamp(bornTimeStamp);

  // 10 BORNHOST
  int bornHostLength = (sysFlag & MessageSysFlag::BORNHOST_V6_FLAG) == 0 ? kIPv4AddrSize : kIPv6AddrSize;
  ByteArray bornHost(bornHostLength);
  byteBuffer.get(bornHost, 0, bornHostLength);
  int32_t bornPort = byteBuffer.getInt();
  msgExt->set_born_host(IPPortToSockaddr(bornHost, bornPort));

  // 11 STORETIMESTAMP
  int64_t storeTimestamp = byteBuffer.getLong();
  msgExt->set_store_timestamp(storeTimestamp);

  // 12 STOREHOST
  int storehostIPLength = (sysFlag & MessageSysFlag::STOREHOST_V6_FLAG) == 0 ? kIPv4AddrSize : kIPv6AddrSize;
  ByteArray storeHost(bornHostLength);
  byteBuffer.get(storeHost, 0, storehostIPLength);
  int32_t storePort = byteBuffer.getInt();
  msgExt->set_store_host(IPPortToSockaddr(storeHost, storePort));

  // 13 RECONSUMETIMES
  int32_t reconsumeTimes = byteBuffer.getInt();
  msgExt->set_reconsume_times(reconsumeTimes);

  // 14 Prepared Transaction Offset
  int64_t preparedTransactionOffset = byteBuffer.getLong();
  msgExt->set_prepared_transaction_offset(preparedTransactionOffset);

  // 15 BODY
  int uncompress_failed = false;
  int32_t bodyLen = byteBuffer.getInt();
  if (bodyLen > 0) {
    if (readBody) {
      ByteArray body(byteBuffer.array() + byteBuffer.arrayOffset() + byteBuffer.position(), bodyLen);
      byteBuffer.position(byteBuffer.position() + bodyLen);

      // decompress body
      if (deCompressBody && (sysFlag & MessageSysFlag::COMPRESSED_FLAG) == MessageSysFlag::COMPRESSED_FLAG) {
        std::string origin_body;
        if (UtilAll::inflate(body, origin_body)) {
          msgExt->set_body(std::move(origin_body));
        } else {
          uncompress_failed = true;
        }
      } else {
        msgExt->set_body(std::string(body.array(), body.size()));
      }
    } else {
      // skip body
      byteBuffer.position(byteBuffer.position() + bodyLen);
    }
  }

  // 16 TOPIC
  int8_t topicLen = byteBuffer.get();
  ByteArray topic(topicLen);
  byteBuffer.get(topic);
  msgExt->set_topic(topic.array(), topic.size());

  // 17 properties
  int16_t propertiesLen = byteBuffer.getShort();
  if (propertiesLen > 0) {
    ByteArray properties(propertiesLen);
    byteBuffer.get(properties);
    std::string propertiesString(properties.array(), properties.size());
    std::map<std::string, std::string> propertiesMap = string2messageProperties(propertiesString);
    MessageAccessor::setProperties(*msgExt, std::move(propertiesMap));
  }

  // 18 msg ID
  std::string msgId = createMessageId(msgExt->store_host(), (int64_t)msgExt->commit_log_offset());
  msgExt->MessageExtImpl::set_msg_id(msgId);

  if (uncompress_failed) {
    LOG_WARN_NEW("can not uncompress message, id:{}", msgExt->msg_id());
  }

  return msgExt;
}

std::vector<MessageExtPtr> MessageDecoder::decodes(ByteBuffer& byteBuffer) {
  return decodes(byteBuffer, true);
}

std::vector<MessageExtPtr> MessageDecoder::decodes(ByteBuffer& byteBuffer, bool readBody) {
  std::vector<MessageExtPtr> msgExts;
  while (byteBuffer.hasRemaining()) {
    auto msgExt = clientDecode(byteBuffer, readBody);
    if (nullptr == msgExt) {
      break;
    }
    msgExts.emplace_back(std::move(msgExt));
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
  const auto& body = message.body();
  uint32_t bodyLen = body.size();
  std::string properties = MessageDecoder::messageProperties2String(message.properties());
  uint16_t propertiesLength = (int16_t)properties.size();
  uint32_t storeSize = 4                        // 1 TOTALSIZE
                       + 4                      // 2 MAGICCODE
                       + 4                      // 3 BODYCRC
                       + 4                      // 4 FLAG
                       + 4 + bodyLen            // 5 BODY
                       + 2 + propertiesLength;  // 6 PROPERTIES

  // TOTALSIZE|MAGICCODE|BODYCRC|FLAG|BodyLen|Body|propertiesLength|properties
  std::string encodeMsg;

  // 1 TOTALSIZE
  uint32_t storeSize_net = ByteOrderUtil::NorminalBigEndian(storeSize);
  encodeMsg.append((char*)&storeSize_net, sizeof(uint32_t));

  // 2 MAGICCODE
  uint32_t magicCode = 0;
  uint32_t magicCode_net = ByteOrderUtil::NorminalBigEndian(magicCode);
  encodeMsg.append((char*)&magicCode_net, sizeof(uint32_t));

  // 3 BODYCRC
  uint32_t bodyCrc = 0;
  uint32_t bodyCrc_net = ByteOrderUtil::NorminalBigEndian(bodyCrc);
  encodeMsg.append((char*)&bodyCrc_net, sizeof(uint32_t));

  // 4 FLAG
  uint32_t flag = message.flag();
  uint32_t flag_net = ByteOrderUtil::NorminalBigEndian(flag);
  encodeMsg.append((char*)&flag_net, sizeof(uint32_t));

  // 5 BODY
  uint32_t bodyLen_net = ByteOrderUtil::NorminalBigEndian(bodyLen);
  encodeMsg.append((char*)&bodyLen_net, sizeof(uint32_t));
  encodeMsg.append(body.data(), body.size());

  // 6 properties
  uint16_t propertiesLength_net = ByteOrderUtil::NorminalBigEndian(propertiesLength);
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
