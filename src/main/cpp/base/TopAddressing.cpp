#include "TopAddressing.h"

#include "absl/strings/match.h"
#include "absl/strings/str_split.h"
#include <spdlog/spdlog.h>
#include <utility>

#include "HttpClient.h"

ROCKETMQ_NAMESPACE_BEGIN

TopAddressing::TopAddressing(std::string host, int port, std::string path)
    : host_(std::move(host)), port_(port), path_(std::move(path)) {}

bool TopAddressing::fetchNameServerAddresses(std::vector<std::string>& list) {
  SPDLOG_DEBUG("Prepare to send HTTP request, timeout=3s");
  std::string base(fmt::format("http://{}:{}", host_, port_));
  // Append host info if necessary.
  std::string query_string(path_);

  if (!unit_name_.empty()) {
    query_string.append("-").append(unit_name_);
  }

  if (absl::StrContains(query_string, "?")) {
    query_string.append("&");
  } else {
    query_string.append("?");
  }

  if (host_info_.hasHostInfo()) {
    query_string.append(host_info_.queryString());
  } else {
    query_string.append("nofix=1");
  }

  std::string request(fmt::format("{}{}", base, query_string));
  std::string response;

  HttpClient http_client;
  if (!http_client) {
    SPDLOG_WARN("Failed to create CURL session. OS probably runs out of resources. Check out memory and FD usage");
    return false;
  }

  if (http_client.get(request, response, absl::Seconds(3))) {
    SPDLOG_INFO("Received name server list: {}", response);
    std::vector<std::string> splits = absl::StrSplit(response, ";");
    for (const auto& item : splits) {
      list.push_back(item);
    }
    return true;
  }

  return false;
}

ROCKETMQ_NAMESPACE_END