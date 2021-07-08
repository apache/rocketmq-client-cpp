#include "absl/time/time.h"
#include <gtest/gtest.h>

TEST(AbseilTimeTest, testTimeFormat) {
  auto now = absl::FromChrono(std::chrono::system_clock::now());
  auto utc_time_zone = absl::UTCTimeZone();
  auto local_time_zone = absl::LocalTimeZone();
  const char* time_format = "%Y%m%dT%H%M%SZ";
  std::cout << "UTC: " << absl::FormatTime(time_format, now, utc_time_zone) << std::endl;
  std::cout << "Beijing Time: " << absl::FormatTime(time_format, now, local_time_zone) << std::endl;
}