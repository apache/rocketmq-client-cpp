#include "Signature.h"
#include "ClientConfigImpl.h"
#include "MetadataConstants.h"
#include "Protocol.h"
#include "TlsHelper.h"
#include "spdlog/spdlog.h"

ROCKETMQ_NAMESPACE_BEGIN

void Signature::sign(ClientConfig* client, absl::flat_hash_map<std::string, std::string>& metadata) {
  assert(client);

  metadata.insert({MetadataConstants::LANGUAGE_KEY, "CPP"});
  // Add common headers
  metadata.insert({MetadataConstants::CLIENT_VERSION_KEY, ClientConfigImpl::CLIENT_VERSION});
  metadata.insert({MetadataConstants::PROTOCOL_VERSION_KEY, Protocol::PROTOCOL_VERSION});

  if (!client->tenantId().empty()) {
    metadata.insert({MetadataConstants::TENANT_ID_KEY, client->tenantId()});
  }

  if (!client->resourceNamespace().empty()) {
    metadata.insert({MetadataConstants::NAMESPACE_KEY, client->resourceNamespace()});
  }

  absl::Time now = absl::Now();
  absl::TimeZone utc_time_zone = absl::UTCTimeZone();
  const std::string request_date_time = absl::FormatTime(MetadataConstants::DATE_TIME_FORMAT, now, utc_time_zone);
  metadata.insert({MetadataConstants::DATE_TIME_KEY, request_date_time});

  if (client->credentialsProvider()) {
    Credentials&& credentials = client->credentialsProvider()->getCredentials();
    if (credentials.accessKey().empty() || credentials.accessSecret().empty()) {
      SPDLOG_WARN("Access credential is incomplete. Check your access key/secret.");
      return;
    }

    std::string authorization;
    authorization.append(MetadataConstants::ALGORITHM_KEY)
        .append(" ")
        .append(MetadataConstants::CREDENTIAL_KEY)
        .append("=")
        .append(credentials.accessKey())
        .append("/")
        .append(client->region())
        .append("/")
        .append(client->serviceName())
        .append(", ")
        .append(MetadataConstants::SIGNED_HEADERS_KEY)
        .append("=")
        .append(MetadataConstants::DATE_TIME_KEY)
        .append(", ")
        .append(MetadataConstants::SIGNATURE_KEY)
        .append("=")
        .append(TlsHelper::sign(credentials.accessSecret(), request_date_time));
    SPDLOG_DEBUG("Add authorization header: {}", authorization);
    metadata.insert({MetadataConstants::AUTHORIZATION, authorization});

    if (!credentials.sessionToken().empty()) {
      metadata.insert({MetadataConstants::STS_SESSION_TOKEN, credentials.sessionToken()});
    }
  }
}

ROCKETMQ_NAMESPACE_END