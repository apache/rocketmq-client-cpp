#ifndef __BATCHMESSAGE_H__
#define __BATCHMESSAGE_H__
#include "MQMessage.h"
#include <string>
namespace rocketmq {
    class BatchMessage : public MQMessage {
    public:
        static std::string encode(std::vector <MQMessage> &msgs);
        static std::string encode(MQMessage &message);
    };
}
#endif