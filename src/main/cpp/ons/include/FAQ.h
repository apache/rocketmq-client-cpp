#pragma once

#include <sstream>
#include <string>

#include "ons/ONSClient.h"

ONS_NAMESPACE_BEGIN

class FAQ {
public:
  FAQ() = default;
  virtual ~FAQ() = default;

  static const std::string FIND_NS_FAILED;
  static const std::string CONNECT_BROKER_FAILED;
  static const std::string SEND_MSG_TO_BROKER_TIMEOUT;
  static const std::string SERVICE_STATE_WRONG;
  static const std::string BROKER_RESPONSE_EXCEPTION;
  static const std::string CLIENT_CHECK_MSG_EXCEPTION;
  static const std::string TOPIC_ROUTE_NOT_EXIST;

  static std::string errorMessage(const std::string& error_message, const std::string& url) {
    std::stringstream ss;
    ss << error_message << "\nSee " << url << " for further details.";
    return ss.str();
  }
};

ONS_NAMESPACE_END