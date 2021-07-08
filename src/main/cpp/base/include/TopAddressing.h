#pragma once

#include <cassert>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

#include "HostInfo.h"
#include "HttpClient.h"
#include "rocketmq/RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

class TopAddressing {
public:
  explicit TopAddressing() : host_("jmenv.tbsite.net"), path_("/rocketmq/nsaddr") {}

  TopAddressing(std::string host, int port, std::string path);

  bool fetchNameServerAddresses(std::vector<std::string>& list);

  void setUnitName(std::string unit_name) { unit_name_ = std::move(unit_name); }

private:
  std::string host_;
  int port_{8080};
  std::string path_;
  HostInfo host_info_;

  /**
   * Allow use to override unit name.
   */
  std::string unit_name_;
};

using TopAddressingPtr = std::unique_ptr<TopAddressing>;

ROCKETMQ_NAMESPACE_END