#pragma once

#include "rocketmq/RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

class MetadataConstants {
public:
  static const char* REQUEST_ID_KEY;
  static const char* TENANT_ID_KEY;
  static const char* NAMESPACE_KEY;
  static const char* AUTHORIZATION;
  static const char* STS_SESSION_TOKEN;
  static const char* DATE_TIME_KEY;
  static const char* ALGORITHM_KEY;
  static const char* CREDENTIAL_KEY;
  static const char* SIGNED_HEADERS_KEY;
  static const char* SIGNATURE_KEY;
  static const char* DATE_TIME_FORMAT;
  static const char* LANGUAGE_KEY;
  static const char* CLIENT_VERSION_KEY;
  static const char* PROTOCOL_VERSION_KEY;
};

ROCKETMQ_NAMESPACE_END