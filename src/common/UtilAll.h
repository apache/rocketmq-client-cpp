/*
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef __UTILALL_H__
#define __UTILALL_H__

#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#ifndef WIN32
#include <pwd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#endif
#include <boost/algorithm/string/trim.hpp>
#include <boost/asio.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/locale/conversion.hpp>
#include <boost/locale/encoding.hpp>
#include <fstream>
#include <map>
#include <sstream>
#include <type_traits>
#include "RocketMQClient.h"

using namespace std;
namespace rocketmq {
//<!************************************************************************
const string null = "";
const string WHITESPACE = " \t\r\n";
const string SUB_ALL = "*";
const string DEFAULT_TOPIC = "TBW102";
const string BENCHMARK_TOPIC = "BenchmarkTest";
const string DEFAULT_PRODUCER_GROUP = "DEFAULT_PRODUCER";
const string DEFAULT_CONSUMER_GROUP = "DEFAULT_CONSUMER";
const string TOOLS_CONSUMER_GROUP = "TOOLS_CONSUMER";
const string CLIENT_INNER_PRODUCER_GROUP = "CLIENT_INNER_PRODUCER";
const string SELF_TEST_TOPIC = "SELF_TEST_TOPIC";
const string RETRY_GROUP_TOPIC_PREFIX = "%RETRY%";
const string DLQ_GROUP_TOPIC_PREFIX = "%DLQ%";
const string ROCKETMQ_HOME_ENV = "ROCKETMQ_HOME";
const string ROCKETMQ_HOME_PROPERTY = "rocketmq.home.dir";
const string MESSAGE_COMPRESS_LEVEL = "rocketmq.message.compressLevel";
const string DEFAULT_SSL_PROPERTY_FILE = "/etc/rocketmq/tls.properties";
const string DEFAULT_CLIENT_KEY_PASSWD = null;
const string DEFAULT_CLIENT_KEY_FILE = "/etc/rocketmq/client.key";
const string DEFAULT_CLIENT_CERT_FILE = "/etc/rocketmq/client.pem";
const string DEFAULT_CA_CERT_FILE = "/etc/rocketmq/ca.pem";
const string WS_ADDR =
    "please set nameserver domain by setDomainName, there is no default "
    "nameserver domain";
const int POLL_NAMESERVER_INTEVAL = 1000 * 30;
const int HEARTBEAT_BROKER_INTERVAL = 1000 * 30;
const int PERSIST_CONSUMER_OFFSET_INTERVAL = 1000 * 5;
const int LINE_SEPARATOR = 1;   // rocketmq::UtilAll::charToString((char) 1);
const int WORD_SEPARATOR = 2;   // rocketmq::UtilAll::charToString((char) 2);
const int HTTP_TIMEOUT = 3000;  // 3S
const int HTTP_CONFLICT = 409;
const int HTTP_OK = 200;
const int HTTP_NOTFOUND = 404;
const int CONNETERROR = -1;
const int MASTER_ID = 0;

template <typename Type>
inline void deleteAndZero(Type& pointer) {
  delete pointer;
  pointer = NULL;
}
#define EMPTY_STR_PTR(ptr) (ptr == NULL || ptr[0] == '\0')
#ifdef WIN32
#define SIZET_FMT "%lu"
#else
#define SIZET_FMT "%zu"
#endif

namespace detail {

template <typename T, bool = false>
struct UseStdToString {
  typedef std::false_type type;
};

template <typename T>
struct UseStdToString<T, true> {
  typedef std::true_type type;
};

template <typename T>
inline std::string to_string(const T& v, std::false_type) {
  std::ostringstream stm;
  stm << v;
  return stm.str();
}

template <typename T>
inline std::string to_string(const T& v, std::true_type) {
  return std::to_string(v);
}

template <typename T>
inline std::string to_string(const T& v) {
  return to_string(v, typename UseStdToString < T,
                   std::is_arithmetic<T>::value && !std::is_same<bool, typename std::decay<T>::type>::value > ::type{});
}

template <>
inline std::string to_string<bool>(const bool& v) {
  return v ? "true" : "false";
}

}  // namespace detail

//<!************************************************************************
class UtilAll {
 public:
  static bool startsWith_retry(const string& topic);
  static string getRetryTopic(const string& consumerGroup);

  static void Trim(string& str);
  static bool isBlank(const string& str);
  static uint64 hexstr2ull(const char* str);
  static int64 str2ll(const char* str);
  static string bytes2string(const char* bytes, int len);

  template <typename T>
  static string to_string(const T& n) {
    return detail::to_string(n);
  }

  static bool to_bool(std::string const& s) { return atoi(s.c_str()); }

  static bool SplitURL(const string& serverURL, string& addr, short& nPort);
  static int Split(vector<string>& ret_, const string& strIn, const char sep);
  static int Split(vector<string>& ret_, const string& strIn, const string& sep);

  static bool StringToInt32(const std::string& str, int32_t& out);
  static bool StringToInt64(const std::string& str, int64_t& val);

  static string getLocalHostName();
  static string getLocalAddress();
  static string getHomeDirectory();

  static string getProcessName();

  static uint64_t currentTimeMillis();
  static uint64_t currentTimeSeconds();

  static bool deflate(std::string& input, std::string& out, int level);
  static bool inflate(std::string& input, std::string& out);
  // Renames file |from_path| to |to_path|. Both paths must be on the same
  // volume, or the function will fail. Destination file will be created
  // if it doesn't exist. Prefer this function over Move when dealing with
  // temporary files. On Windows it preserves attributes of the target file.
  // Returns true on success.
  // Returns false on failure..
  static bool ReplaceFile(const std::string& from_path, const std::string& to_path);

  static std::map<std::string, std::string> ReadProperties(const std::string& path);

 private:
  static std::string s_localHostName;
  static std::string s_localIpAddress;
};
//<!***************************************************************************
}  // namespace rocketmq
#endif
