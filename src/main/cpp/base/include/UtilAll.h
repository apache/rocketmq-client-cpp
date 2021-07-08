#pragma once

#include <string>
#include <vector>

#include "rocketmq/RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

const std::string DEFAULT_CONSUMER_GROUP = "DEFAULT_CONSUMER";

class UtilAll {
public:
  static std::string getHostIPv4();

  /**
   * Walk through all network interfaces and return IPv4 addresses.
   */
  static std::vector<std::string> getIPv4Addresses();

  /**
   * Select IP from provided lists. The selecting criteria is first WAN then LAN.
   * For case of LAN, the following order is followed:
   * 1, 10.0.0.0/8 (255.0.0.0),          10.0.0.0 – 10.255.255.255
   * 2, 100.64.0.0/10(255.192.0.0),    100.64.0.0 - 100.127.255.255
   * 3, 172.16.0.0/12 (255.240.0.0)    172.16.0.0 – 172.31.255.255
   * 4, 192.168.0.0/16 (255.255.0.0), 192.168.0.0 – 192.168.255.255
   */
  static bool pickIPv4Address(const std::vector<std::string>& addresses, std::string& result);

  static std::string LOOP_BACK_IP;

  static void int16tobyte(char* out_array, int16_t data);

  static void int32tobyte(char* out_array, int32_t data);

  /**
   * Compress
   * @param src
   * @param dst
   * @return
   */
  static bool compress(const std::string& src, std::string& dst);

  static bool uncompress(const std::string& src, std::string& dst);

private:
  static int findCategory(const std::string& ip, const std::vector<std::pair<std::string, std::string>>& ranges);

  static std::vector<std::pair<std::string, std::string>> Ranges;
};

ROCKETMQ_NAMESPACE_END