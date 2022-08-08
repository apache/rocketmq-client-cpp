#pragma once

#include "ONSClientException.h"
#include "SendResultONS.h"

ONS_NAMESPACE_BEGIN

class SendCallbackONS {
public:
  virtual ~SendCallbackONS() = default;

  virtual void onSuccess(SendResultONS& send_result) = 0;

  virtual void onException(ONSClientException& e) = 0;
};

ONS_NAMESPACE_END