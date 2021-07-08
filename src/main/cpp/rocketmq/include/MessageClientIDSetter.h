#pragma once
#include "UtilAll.h"
#include "sstream"
#ifndef WIN32
#include <sys/types.h>
#include <unistd.h>
#endif
#include "LoggerImpl.h"
#include "UtilAll.h"
#include "absl/base/thread_annotations.h"
#include "absl/synchronization/mutex.h"
#include <cstdint>
#include <ctime>
#include <iostream>
#include <string>

#ifdef WIN32
#include <windows.h>
#endif

ROCKETMQ_NAMESPACE_BEGIN

class MessageClientIDSetter {
public:
  MessageClientIDSetter() = default;

  static std::string createUniqId() LOCKS_EXCLUDED(mtx_);

  static void setStartTime(std::chrono::system_clock::time_point time_p) {
    time_t now = std::chrono::system_clock::to_time_t(time_p);
    struct tm* p = localtime(&now);
    p->tm_sec = 0;
    p->tm_min = 0;
    p->tm_hour = 0;
    p->tm_mday = 1;
    m_startTime = std::chrono::system_clock::from_time_t(mktime(p));
    p->tm_mon = p->tm_mon + 1; // next month
    // mktime auto adjust time
    m_nextStartTime = std::chrono::system_clock::from_time_t(mktime(p));
  }

  static std::string bytes2string(const char* bytebuffer, int arr_len) {
    const auto* byte_arr = reinterpret_cast<const unsigned char*>(bytebuffer);
    std::ostringstream ret;
    for (int i = 0; i < arr_len; ++i) {
      char hex1;
      char hex2;
      int value = byte_arr[i];
      // parse 16
      int v1 = value / 16;
      int v2 = value % 16;
      if (v1 >= 0 && v1 <= 9) {
        hex1 = static_cast<char>(48 + v1);
      } else {
        hex1 = static_cast<char>(55 + v1);
      }

      if (v2 >= 0 && v2 <= 9) {
        hex2 = static_cast<char>(48 + v2);
      } else {
        hex2 = static_cast<char>(55 + v2);
      }

      ret << hex1;
      ret << hex2;
    }

    return ret.str();
  }

private:
  static char data[16];
  static int32_t hash_code;
  static int16_t counter;
  static int16_t basePos;
  static std::chrono::system_clock::time_point m_startTime;
  static std::chrono::system_clock::time_point m_nextStartTime;
  static absl::Mutex mtx_;
};

ROCKETMQ_NAMESPACE_END