#pragma once

#include <string>

namespace rocketmq {

struct HostInfo {
  std::string site_;
  std::string unit_;
  std::string app_;
  std::string stage_;

  explicit HostInfo();

  bool hasHostInfo() const;

  std::string queryString() const;

  static const char* ENV_LABEL_SITE;
  static const char* ENV_LABEL_UNIT;
  static const char* ENV_LABEL_APP;
  static const char* ENV_LABEL_STAGE;

private:
  static void getEnv(const char* env, std::string& holder);

  static void appendLabel(std::string& query_string, const char* key, const std::string& value);
};
} // namespace rocketmq