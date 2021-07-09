#include "Signature.h"
#include "Metadata.h"
#include "Protocol.h"
#include "TlsHelper.h"
#include "spdlog/spdlog.h"

ROCKETMQ_NAMESPACE_BEGIN

void Signature::sign(ClientConfig* client, absl::flat_hash_map<std::string, std::string>& metadata) {
  assert(client);

  metadata.insert({Metadata::LANGUAGE_KEY, "CPP"});
  // Add common headers
  metadata.insert({Metadata::CLIENT_VERSION_KEY, ClientConfig::CLIENT_VERSION});
  metadata.insert({Metadata::PROTOCOL_VERSION_KEY, Protocol::PROTOCOL_VERSION});

  if (!client->tenantId().empty()) {
    metadata.insert({Metadata::TENANT_ID_KEY, client->tenantId()});
  }

  if (!client->arn().empty()) {
    metadata.insert({Metadata::ARN_KEY, client->arn()});
  }

  absl::Time now = absl::Now();
  absl::TimeZone utc_time_zone = absl::UTCTimeZone();
  const std::string request_date_time = absl::FormatTime(Metadata::DATE_TIME_FORMAT, now, utc_time_zone);
  metadata.insert({Metadata::DATE_TIME_KEY, request_date_time});

  if (client->credentialsProvider()) {
    Credentials&& credentials = client->credentialsProvider()->getCredentials();
    if (credentials.accessKey().empty() || credentials.accessSecret().empty()) {
      SPDLOG_WARN("Access credential is incomplete. Check your access key/secret.");
      return;
    }

    std::string authorization;
    authorization.append(Metadata::ALGORITHM_KEY)
        .append(" ")
        .append(Metadata::CREDENTIAL_KEY)
        .append("=")
        .append(credentials.accessKey())
        .append("/")
        .append(client->region())
        .append("/")
        .append(client->serviceName())
        .append(", ")
        .append(Metadata::SIGNED_HEADERS_KEY)
        .append("=")
        .append(Metadata::DATE_TIME_KEY)
        .append(", ")
        .append(Metadata::SIGNATURE_KEY)
        .append("=")
        .append(TlsHelper::sign(credentials.accessSecret(), request_date_time));
    SPDLOG_DEBUG("Add authorization header: {}", authorization);
    metadata.insert({Metadata::AUTHORIZATION, authorization});
  }
}

ROCKETMQ_NAMESPACE_END