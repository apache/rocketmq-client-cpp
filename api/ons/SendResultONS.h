#pragma once
#include <string>

#include "ONSClient.h"

ONS_NAMESPACE_BEGIN

class SendCallbackONSWrapper;
class ONSSendCallback;
class ProducerImpl;
class OrderProducerImpl;
class TransactionProducerImpl;

class ONSCLIENT_API SendResultONS {
public:
  SendResultONS();

  virtual ~SendResultONS();

  const std::string& getMessageId() const;

  operator bool() {
    return !message_id_.empty();
  }

protected:
  void setMessageId(const std::string& message_id);

private:
  std::string message_id_;

  friend class SendCallbackONSWrapper;
  friend class ONSSendCallback;
  friend class ProducerImpl;
  friend class OrderProducerImpl;
  friend class TransactionProducerImpl;
};

ONS_NAMESPACE_END