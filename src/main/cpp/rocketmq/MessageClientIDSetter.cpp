#include "MessageClientIDSetter.h"
#include "spdlog/spdlog.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

ROCKETMQ_NAMESPACE_BEGIN

absl::Mutex MessageClientIDSetter::mtx_;
char MessageClientIDSetter::data[16] = {0};
int16_t MessageClientIDSetter::counter = 0;
int16_t MessageClientIDSetter::basePos = 0;
int32_t MessageClientIDSetter::hash_code = 0;
std::chrono::system_clock::time_point MessageClientIDSetter::m_startTime = std::chrono::system_clock::now();
std::chrono::system_clock::time_point MessageClientIDSetter::m_nextStartTime = std::chrono::system_clock::now();

std::string MessageClientIDSetter::createUniqId() {
  absl::MutexLock lock(&mtx_);
  char tmp_buf[4];
  static bool initialized = false;
  if (!initialized) { // only first execute
    std::string&& ip = UtilAll::getHostIPv4();
    struct in_addr addr = {};
    if (inet_aton(ip.c_str(), &addr)) {
      memcpy(data, &(addr.s_addr), sizeof(addr.s_addr));
    } else {
      SPDLOG_ERROR("inet_aton failed. IP: {}", ip);
    }

    // TO-DO: WIN32
    UtilAll::int16tobyte(tmp_buf, static_cast<int16_t>(getpid()));
    memcpy(data + 4, tmp_buf, 2);

    UtilAll::int32tobyte(tmp_buf, static_cast<int32_t>(hash_code++));
    memcpy(data + 6, tmp_buf, 4);

    setStartTime(std::chrono::system_clock::now());
    initialized = true;
  }

  UtilAll::int32tobyte(tmp_buf, static_cast<int32_t>(hash_code++));
  memcpy(data + 6, tmp_buf, 4);
  std::chrono::system_clock::time_point current = std::chrono::system_clock::now();
  if (current.time_since_epoch() >= m_nextStartTime.time_since_epoch()) {
    setStartTime(current);
  }
  int64_t time_duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - m_startTime).count();
  UtilAll::int32tobyte(tmp_buf, static_cast<int32_t>(time_duration));
  memcpy(data + 10, tmp_buf, 4);

  UtilAll::int16tobyte(tmp_buf, counter++);
  memcpy(data + 14, tmp_buf, 2);

  return bytes2string(data, 16);
}

ROCKETMQ_NAMESPACE_END