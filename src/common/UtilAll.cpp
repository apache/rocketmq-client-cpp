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
#include "UtilAll.h"

#include <chrono>
#include <iostream>

#ifndef WIN32
#include <netdb.h>     // gethostbyname
#include <pwd.h>       // getpwuid
#include <sys/stat.h>  // mkdir
#include <unistd.h>    // gethostname, access, getpid
#else
#include <Winsock2.h>
#include <direct.h>
#include <io.h>
#endif

#include <zlib.h>

#define ZLIB_CHUNK 16384

#include "Logging.h"
#include "SocketUtil.h"

namespace rocketmq {

std::string UtilAll::s_localHostName;
std::string UtilAll::s_localIpAddress;

bool UtilAll::try_lock_for(std::timed_mutex& mutex, long timeout) {
  auto now = std::chrono::steady_clock::now();
  auto deadline = now + std::chrono::milliseconds(timeout);
  for (;;) {
    if (mutex.try_lock_until(deadline)) {
      return true;
    }
    now = std::chrono::steady_clock::now();
    if (now > deadline) {
      return false;
    }
    std::this_thread::yield();
  }
}

int32_t UtilAll::HashCode(const std::string& str) {
  // FIXME: don't equal to String#hashCode in Java
  int32_t h = 0;
  if (!str.empty()) {
    for (const auto& c : str) {
      h = 31 * h + c;
    }
  }
  return h;
}

static const int hex2int[256] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,
    -1, -1, -1, -1, -1, -1, -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

uint64_t UtilAll::hexstr2ull(const char* str) {
  uint64_t num = 0;
  unsigned char* ch = (unsigned char*)str;
  while (*ch != '\0') {
    num = (num << 4) + hex2int[*ch];
    ch++;
  }
  return num;
}

static const char sHexAlphabet[] = "0123456789ABCDEF";

std::string UtilAll::bytes2string(const char* bytes, size_t len) {
  if (bytes == nullptr || len <= 0) {
    return std::string();
  }

  std::string buffer;
  buffer.reserve(len * 2 + 1);
  for (std::size_t i = 0; i < len; i++) {
    unsigned char v = (unsigned char)bytes[i];
    buffer.append(1, sHexAlphabet[v >> 4]);
    buffer.append(1, sHexAlphabet[v & 0x0FU]);
  }
  return buffer;
}

bool UtilAll::isRetryTopic(const std::string& topic) {
  return topic.find(RETRY_GROUP_TOPIC_PREFIX) == 0;
}

std::string UtilAll::getRetryTopic(const std::string& consumerGroup) {
  return RETRY_GROUP_TOPIC_PREFIX + consumerGroup;
}

std::string UtilAll::getReplyTopic(const std::string& clusterName) {
  return clusterName + "_" + REPLY_TOPIC_POSTFIX;
}

void UtilAll::Trim(std::string& str) {
  str.erase(0, str.find_first_not_of(' '));  // prefixing spaces
  str.erase(str.find_last_not_of(' ') + 1);  // surfixing spaces
}

bool UtilAll::isBlank(const std::string& str) {
  if (str.empty()) {
    return true;
  }

  std::string::size_type left = str.find_first_not_of(WHITESPACE);

  if (left == std::string::npos) {
    return true;
  }

  return false;
}

bool UtilAll::SplitURL(const std::string& serverURL, std::string& addr, short& nPort) {
  auto pos = serverURL.find(':');
  if (pos == std::string::npos) {
    return false;
  }

  addr = serverURL.substr(0, pos);
  if (0 == addr.compare("localhost")) {
    addr = "127.0.0.1";
  }

  pos++;
  std::string port = serverURL.substr(pos, serverURL.length() - pos);
  nPort = atoi(port.c_str());
  if (nPort == 0) {
    return false;
  }
  return true;
}

int UtilAll::Split(std::vector<std::string>& ret_, const std::string& strIn, const char sep) {
  if (strIn.empty())
    return 0;

  std::string tmp;
  std::string::size_type pos_begin = strIn.find_first_not_of(sep);
  std::string::size_type comma_pos = 0;

  while (pos_begin != std::string::npos) {
    comma_pos = strIn.find(sep, pos_begin);
    if (comma_pos != std::string::npos) {
      tmp = strIn.substr(pos_begin, comma_pos - pos_begin);
      pos_begin = comma_pos + 1;
    } else {
      tmp = strIn.substr(pos_begin);
      pos_begin = comma_pos;
    }

    if (!tmp.empty()) {
      ret_.push_back(tmp);
      tmp.clear();
    }
  }
  return ret_.size();
}

int UtilAll::Split(std::vector<std::string>& ret_, const std::string& strIn, const std::string& sep) {
  if (strIn.empty())
    return 0;

  std::string tmp;
  std::string::size_type pos_begin = strIn.find_first_not_of(sep);
  std::string::size_type comma_pos = 0;

  while (pos_begin != std::string::npos) {
    comma_pos = strIn.find(sep, pos_begin);
    if (comma_pos != std::string::npos) {
      tmp = strIn.substr(pos_begin, comma_pos - pos_begin);
      pos_begin = comma_pos + sep.length();
    } else {
      tmp = strIn.substr(pos_begin);
      pos_begin = comma_pos;
    }

    if (!tmp.empty()) {
      ret_.push_back(tmp);
      tmp.clear();
    }
  }
  return ret_.size();
}

std::string UtilAll::getLocalHostName() {
  if (s_localHostName.empty()) {
    char name[1024];
    if (::gethostname(name, sizeof(name)) != 0) {
      return null;
    }
    s_localHostName.append(name, strlen(name));
  }
  return s_localHostName;
}

std::string UtilAll::getLocalAddress() {
  if (s_localIpAddress.empty()) {
    auto hostname = getLocalHostName();
    if (!hostname.empty()) {
      try {
        s_localIpAddress = socketAddress2String(lookupNameServers(hostname));
      } catch (std::exception& e) {
        LOG_WARN(e.what());
        s_localIpAddress = "127.0.0.1";
      }
    }
  }
  return s_localIpAddress;
}

uint32_t UtilAll::getIP() {
  std::string ip = UtilAll::getLocalAddress();
  if (ip.empty()) {
    return 0;
  }

  char* ip_str = new char[ip.length() + 1];
  std::strncpy(ip_str, ip.c_str(), ip.length());
  ip_str[ip.length()] = '\0';

  int i = 3;
  uint32_t nResult = 0;
  for (char* token = std::strtok(ip_str, "."); token != nullptr && i >= 0; token = std::strtok(nullptr, ".")) {
    uint32_t n = std::atoi(token);
    nResult |= n << (8 * i--);
  }

  delete[] ip_str;

  return nResult;
}

std::string UtilAll::getHomeDirectory() {
#ifndef WIN32
  char* homeEnv = std::getenv("HOME");
  std::string homeDir;
  if (homeEnv == NULL) {
    homeDir.append(getpwuid(getuid())->pw_dir);
  } else {
    homeDir.append(homeEnv);
  }
#else
  std::string homeDir(std::getenv("USERPROFILE"));
#endif
  return homeDir;
}

static bool createDirectoryInner(const char* dir) {
  if (dir == nullptr) {
    std::cerr << "directory is nullptr" << std::endl;
    return false;
  }
  if (access(dir, F_OK) == -1) {
#ifdef _WIN32
    int flag = mkdir(dir);
#else
    int flag = mkdir(dir, 0755);
#endif
    return flag == 0;
  }
  return true;
}

void UtilAll::createDirectory(std::string const& dir) {
  const char* ptr = dir.c_str();
  if (access(ptr, F_OK) == 0) {
    return;
  }
  char buff[2048] = {0};
  for (unsigned int i = 0; i < dir.size(); i++) {
    if (i != 0 && (*(ptr + i) == '/' || *(ptr + i) == '\\')) {
      memcpy(buff, ptr, i);
      createDirectoryInner(buff);
      memset(buff, 0, 1024);
    }
  }
  return;
}

bool UtilAll::existDirectory(std::string const& dir) {
  return access(dir.c_str(), F_OK) == 0;
}

int UtilAll::getProcessId() {
#ifndef WIN32
  return getpid();
#else
  return ::GetCurrentProcessId();
#endif
}

std::string UtilAll::getProcessName() {
#ifndef WIN32
  char buf[PATH_MAX] = {0};
  char procpath[PATH_MAX] = {0};
  int count = PATH_MAX;

  sprintf(procpath, "/proc/%d/exe", getpid());
  if (access(procpath, F_OK) == -1) {
    return "";
  }

  int retval = readlink(procpath, buf, count - 1);
  if ((retval < 0 || retval >= count - 1)) {
    return "";
  }
  if (!strcmp(buf + retval - 10, " (deleted)"))
    buf[retval - 10] = '\0';  // remove last " (deleted)"
  else
    buf[retval] = '\0';

  char* process_name = strrchr(buf, '/');
  if (process_name) {
    return std::string(process_name + 1);
  } else {
    return "";
  }
#else
  TCHAR szFileName[MAX_PATH];
  ::GetModuleFileName(NULL, szFileName, MAX_PATH);
  return std::string(szFileName);
#endif
}

int64_t UtilAll::currentTimeMillis() {
  auto since_epoch =
      std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
  return static_cast<int64_t>(since_epoch.count());
}

int64_t UtilAll::currentTimeSeconds() {
  auto since_epoch =
      std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch());
  return static_cast<int64_t>(since_epoch.count());
}

bool UtilAll::deflate(const std::string& input, std::string& out, int level) {
  return deflate(input.data(), input.length(), out, level);
}

bool UtilAll::deflate(const char* input, size_t len, std::string& out, int level) {
  int ret;
  unsigned have;
  z_stream strm;
  unsigned char buf[ZLIB_CHUNK];

  /* allocate deflate state */
  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;
  strm.opaque = Z_NULL;
  ret = ::deflateInit(&strm, level);
  if (ret != Z_OK) {
    return false;
  }

  strm.avail_in = len;
  strm.next_in = (z_const Bytef*)input;

  /* run deflate() on input until output buffer not full, finish
     compression if all of source has been read in */
  do {
    strm.avail_out = ZLIB_CHUNK;
    strm.next_out = buf;
    ret = ::deflate(&strm, Z_FINISH); /* no bad return value */
    assert(ret != Z_STREAM_ERROR);    /* state not clobbered */
    have = ZLIB_CHUNK - strm.avail_out;
    out.append((char*)buf, have);
  } while (strm.avail_out == 0);
  assert(strm.avail_in == 0);  /* all input will be used */
  assert(ret == Z_STREAM_END); /* stream will be complete */

  /* clean up and return */
  (void)::deflateEnd(&strm);

  return true;
}

bool UtilAll::inflate(const std::string& input, std::string& out) {
  return inflate(input.data(), input.length(), out);
}

bool UtilAll::inflate(const char* input, size_t len, std::string& out) {
  int ret;
  unsigned have;
  z_stream strm;
  unsigned char buf[ZLIB_CHUNK];

  /* allocate inflate state */
  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;
  strm.opaque = Z_NULL;
  strm.avail_in = 0;
  strm.next_in = Z_NULL;
  ret = ::inflateInit(&strm);
  if (ret != Z_OK) {
    return false;
  }

  strm.avail_in = len;
  strm.next_in = (z_const Bytef*)input;

  /* run inflate() on input until output buffer not full */
  do {
    strm.avail_out = ZLIB_CHUNK;
    strm.next_out = buf;
    ret = ::inflate(&strm, Z_NO_FLUSH);
    assert(ret != Z_STREAM_ERROR); /* state not clobbered */
    switch (ret) {
      case Z_NEED_DICT:
        ret = Z_DATA_ERROR; /* and fall through */
      case Z_DATA_ERROR:
      case Z_MEM_ERROR:
        (void)inflateEnd(&strm);
        return false;
    }
    have = ZLIB_CHUNK - strm.avail_out;
    out.append((char*)buf, have);
  } while (strm.avail_out == 0);

  /* clean up and return */
  (void)::inflateEnd(&strm);

  return ret == Z_STREAM_END;
}

bool UtilAll::ReplaceFile(const std::string& from_path, const std::string& to_path) {
#ifdef WIN32
  // Try a simple move first.  It will only succeed when |to_path| doesn't
  // already exist.
  if (::MoveFile(from_path.c_str(), to_path.c_str())) {
    return true;
  }

  // Try the full-blown replace if the move fails, as ReplaceFile will only
  // succeed when |to_path| does exist. When writing to a network share, we may
  // not be able to change the ACLs. Ignore ACL errors then
  // (REPLACEFILE_IGNORE_MERGE_ERRORS).
  if (::ReplaceFile(to_path.c_str(), from_path.c_str(), NULL, REPLACEFILE_IGNORE_MERGE_ERRORS, NULL, NULL)) {
    return true;
  }

  return false;
#else
  return rename(from_path.c_str(), to_path.c_str()) == 0;
#endif
}

}  // namespace rocketmq
