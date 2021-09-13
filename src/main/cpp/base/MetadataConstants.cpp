#include "MetadataConstants.h"

ROCKETMQ_NAMESPACE_BEGIN

const char* MetadataConstants::TENANT_ID_KEY = "x-mq-tenant-id";
const char* MetadataConstants::NAMESPACE_KEY = "x-mq-namespace";
const char* MetadataConstants::AUTHORIZATION = "authorization";
const char* MetadataConstants::STS_SESSION_TOKEN = "x-mq-session-token";
const char* MetadataConstants::DATE_TIME_KEY = "x-mq-date-time";
const char* MetadataConstants::ALGORITHM_KEY = "MQv2-HMAC-SHA1";
const char* MetadataConstants::CREDENTIAL_KEY = "Credential";
const char* MetadataConstants::SIGNED_HEADERS_KEY = "SignedHeaders";
const char* MetadataConstants::SIGNATURE_KEY = "Signature";
const char* MetadataConstants::DATE_TIME_FORMAT = "%Y%m%dT%H%M%SZ";
const char* MetadataConstants::LANGUAGE_KEY = "x-mq-language";
const char* MetadataConstants::CLIENT_VERSION_KEY = "x-mq-client-version";
const char* MetadataConstants::PROTOCOL_VERSION_KEY = "x-mq-protocol-version";
const char* MetadataConstants::REQUEST_ID_KEY = "x-mq-request-id";

ROCKETMQ_NAMESPACE_END