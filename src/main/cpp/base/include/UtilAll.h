#pragma once

#include <string>
#include <vector>

#include "rocketmq/RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

class UtilAll {
public:
  static std::string hostname();

  static bool macAddress(std::vector<unsigned char>& mac);

  /**
   * Compress
   * @param src
   * @param dst
   * @return
   */
  static bool compress(const std::string& src, std::string& dst);

  static bool uncompress(const std::string& src, std::string& dst);

private:
};

ROCKETMQ_NAMESPACE_END