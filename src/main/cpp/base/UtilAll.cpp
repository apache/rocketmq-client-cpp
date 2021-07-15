#include "UtilAll.h"

#include "absl/strings/str_split.h"
#include "LoggerImpl.h"
#include "uv.h"
#include "zlib.h"

namespace rocketmq {

std::string UtilAll::getHostIPv4() {
  static std::string host_ip;
  if (host_ip.empty()) {
    std::vector<std::string> addresses = getIPv4Addresses();
    if (!pickIPv4Address(addresses, host_ip)) {
      return LOOP_BACK_IP;
    }
  }
  return host_ip;
}

std::vector<std::string> UtilAll::getIPv4Addresses() {
  std::vector<std::string> addresses;
  char buf[512];
  uv_interface_address_t* info;
  int count, i;

  uv_interface_addresses(&info, &count);
  i = count;
  while (i--) {
    uv_interface_address_t interface = info[i];
    if (interface.address.address4.sin_family == AF_INET) {
      uv_ip4_name(&interface.address.address4, buf, sizeof(buf));
      if (interface.is_internal) {
        continue;
      }
      addresses.emplace_back(std::string(buf));
    } else if (interface.address.address4.sin_family == AF_INET6) {
      uv_ip6_name(&interface.address.address6, buf, sizeof(buf));
    }
  }
  uv_free_interface_addresses(info, count);
  return addresses;
}

/**
 * Select IP from provided lists. The selecting criteria is first WAN then LAN.
 * For case of LAN, the following order is followed:
 * 1, 10.0.0.0/8 (255.0.0.0),          10.0.0.0 – 10.255.255.255
 * 2, 100.64.0.0/10(255.192.0.0),    100.64.0.0 - 100.127.255.255
 * 3, 172.16.0.0/12 (255.240.0.0)    172.16.0.0 – 172.31.255.255
 * 4, 192.168.0.0/16 (255.255.0.0), 192.168.0.0 – 192.168.255.255
 */
std::vector<std::pair<std::string, std::string>> UtilAll::Ranges = {{"10.0.0.0", "10.255.255.255"},
                                                                    {"100.64.0.0", "100.127.255.255"},
                                                                    {"172.16.0.0", "172.31.255.255"},
                                                                    {"192.168.0.0", "192.168.255.255"}};

int UtilAll::findCategory(const std::string& ip, const std::vector<std::pair<std::string, std::string>>& categories) {
  struct in_addr addr = {};
  int status = inet_aton(ip.data(), &addr);
  if (!status) {
    SPDLOG_WARN("inet_aton failed. IPv4: {}", ip);
    return -1;
  }
  uint32_t current = ntohl(addr.s_addr);

  int index = 1;
  for (const auto& item : categories) {
    const std::string& left = item.first;
    const std::string& right = item.second;
    struct in_addr low = {};
    struct in_addr high = {};
    status = inet_aton(left.data(), &low);
    if (!status) {
      SPDLOG_WARN("inet_aton failed. IPv4: {}", left);
      return -1;
    }

    status = inet_aton(right.data(), &high);
    if (!status) {
      SPDLOG_WARN("inet_aton failed. IPv4: {}", right);
      return -1;
    }

    uint32_t start = ntohl(low.s_addr);
    uint32_t end = ntohl(high.s_addr);
    if (current >= start && current <= end) {
      return index;
    }
    index++;
  }
  return 0;
}

bool UtilAll::pickIPv4Address(const std::vector<std::string>& addresses, std::string& result) {
  if (addresses.empty()) {
    return false;
  }

  if (addresses.size() == 1) {
    result = addresses[0];
  }

  std::vector<std::vector<std::string>> categories;
  categories.reserve(5);
  categories.resize(5);
  for (const auto& ip : addresses) {
    int index = findCategory(ip, Ranges);
    if (index >= 0) {
      std::vector<std::vector<std::string>>::size_type category_index = index;
      if (category_index < categories.size()) {
        categories[category_index].push_back(ip);
      }
    }
  }
  for (const auto& category : categories) {
    if (!category.empty()) {
      result = category[0];
      return true;
    }
  }
  result = addresses[0];
  return true;
}

void UtilAll::int16tobyte(char* out_array, int16_t data) {
  if (!out_array) {
    return;
  }
  out_array[0] = static_cast<char>(data & 0xff);
  out_array[1] = static_cast<char>((data >> 8) & 0xff);
}

void UtilAll::int32tobyte(char* out_array, int32_t data) {
  if (!out_array) {
    return;
  }
  out_array[0] = static_cast<char>(data & 0xff);
  out_array[1] = static_cast<char>((data >> 8) & 0xff);
  out_array[2] = static_cast<char>((data >> 16) & 0xff);
  out_array[3] = static_cast<char>((data >> 24) & 0xff);
}

bool UtilAll::compress(const std::string& src, std::string& dst) {
  z_stream stream;
  stream.zalloc = Z_NULL;
  stream.zfree = Z_NULL;
  stream.opaque = Z_NULL;
  stream.next_in = reinterpret_cast<unsigned char*>(const_cast<char*>(src.c_str()));
  stream.avail_in = src.length();

  deflateInit(&stream, Z_DEFAULT_COMPRESSION);
  uint32_t bound = deflateBound(&stream, src.length());
  std::vector<unsigned char> buffer(bound);
  stream.next_out = buffer.data();
  stream.avail_out = bound;

  int status = deflate(&stream, Z_FINISH);

  // zlib is unexpected wrong.
  if (Z_STREAM_END != status) {
    deflateEnd(&stream);
    return false;
  }

  dst.reserve(stream.total_out);
  assert(stream.total_out == bound - stream.avail_out);
  std::copy(buffer.data(), buffer.data() + stream.total_out, std::back_inserter(dst));
  deflateEnd(&stream);
  return true;
}

bool UtilAll::uncompress(const std::string& src, std::string& dst) {
  int status;
  uint32_t buffer_length = 4096;
  std::vector<unsigned char> buffer(buffer_length);
  z_stream stream;

  // Use default malloc / free allocator.
  stream.zalloc = Z_NULL;
  stream.zfree = Z_NULL;
  stream.opaque = Z_NULL;
  stream.next_in = reinterpret_cast<unsigned char*>(const_cast<char*>(src.c_str()));
  stream.avail_in = src.length();

  inflateInit(&stream);
  stream.next_out = buffer.data();
  stream.avail_out = buffer_length;

  while (true) {
    status = inflate(&stream, Z_SYNC_FLUSH);

    if (Z_STREAM_ERROR == status) {
      return false;
    }
    std::copy(buffer.data(), buffer.data() + buffer_length - stream.avail_out, std::back_inserter(dst));
    stream.avail_out = buffer_length;
    stream.next_out = buffer.data();

    // inflation completed OK
    if (Z_STREAM_END == status) {
      inflateEnd(&stream);
      return true;
    }

    // inflation made some progress
    if (Z_OK == status) {
      continue;
    }

    // Something is wrong
    inflateEnd(&stream);
    return false;
  }
}

std::string UtilAll::LOOP_BACK_IP("127.0.0.1");

} // namespace rocketmq
