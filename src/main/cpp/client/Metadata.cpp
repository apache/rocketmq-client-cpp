#include "Metadata.h"

ROCKETMQ_NAMESPACE_BEGIN

const char* Metadata::TENANT_ID_KEY = "x-mq-tenant-id";
const char* Metadata::ARN_KEY = "x-mq-arn";
const char* Metadata::AUTHORIZATION = "authorization";
const char* Metadata::DATE_TIME_KEY = "x-mq-date-time";
const char* Metadata::ALGORITHM_KEY = "MQv2-HMAC-SHA1";
const char* Metadata::CREDENTIAL_KEY = "Credential";
const char* Metadata::SIGNED_HEADERS_KEY = "SignedHeaders";
const char* Metadata::SIGNATURE_KEY = "Signature";
const char* Metadata::DATE_TIME_FORMAT = "%Y%m%dT%H%M%SZ";
const char* Metadata::LANGUAGE_KEY = "x-mq-language";
const char* Metadata::CLIENT_VERSION_KEY = "x-mq-client-version";
const char* Metadata::PROTOCOL_VERSION_KEY = "x-mq-protocol-version";
const char* Metadata::REQUEST_ID_KEY = "x-mq-request-id";

ROCKETMQ_NAMESPACE_END