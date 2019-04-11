#include "BatchMessage.h"
#include "MQDecoder.h"
#include "StringIdMaker.h"

using namespace std;
namespace rocketmq {

std::string BatchMessage::encode(std::vector<MQMessage>& msgs) {
  string encodedBody;
  for (auto message : msgs) {
    string unique_id = StringIdMaker::get_mutable_instance().get_unique_id();
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
}
