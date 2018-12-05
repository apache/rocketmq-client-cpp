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
#include "StringIdMaker.h"

namespace rocketmq {

#ifdef WIN32
int gettimeofdayWin(struct timeval *tp, void *tzp)
{
	time_t clock;
	struct tm tm;
	SYSTEMTIME wtm;
	GetLocalTime(&wtm);
	tm.tm_year	= wtm.wYear - 1900;
	tm.tm_mon = wtm.wMonth - 1;
	tm.tm_mday = wtm.wDay;
	tm.tm_hour = wtm.wHour;
	tm.tm_min = wtm.wMinute;
	tm.tm_sec = wtm.wSecond;
	tm. tm_isdst = -1;
	clock = mktime(&tm);
	tp->tv_sec = clock;
	tp->tv_usec = wtm.wMilliseconds * 1000;
	return (0);
}
#endif

StringIdMaker::StringIdMaker() {
  memset(_buff, 0, sizeof(_buff));
  memset(_0x_buff, 0, sizeof(_0x_buff));
  srand((uint32_t)time(NULL));
  init_prefix();
}
StringIdMaker::~StringIdMaker() {}

void StringIdMaker::init_prefix() {
  uint32_t pid = getpid();
  uint32_t ip = get_ip();
  uint32_t random_num = (rand() % 0xFFFF);

  memcpy(_buff + 2, &pid, 4);
  memcpy(_buff, &ip, 4);
  memcpy(_buff + 6, &random_num, 4);

  hexdump(_buff, _0x_buff, 10);

  set_start_and_next_tm();
}

uint32_t StringIdMaker::get_ip() {
  char name[1024];
  boost::system::error_code ec;
  if (boost::asio::detail::socket_ops::gethostname(name, sizeof(name), ec) != 0) {
    return 0;
  }

  boost::asio::io_service io_service;
  boost::asio::ip::tcp::resolver resolver(io_service);
  boost::asio::ip::tcp::resolver::query query(name, "");
  boost::system::error_code error;
  boost::asio::ip::tcp::resolver::iterator iter = resolver.resolve(query, error);
  if (error) {
    return 0;
  }
  boost::asio::ip::tcp::resolver::iterator end;  // End marker.
  boost::asio::ip::tcp::endpoint ep;
  while (iter != end) {
    ep = *iter++;
  }
  std::string s_localIpAddress = ep.address().to_string();

  int a[4];
  std::string IP = s_localIpAddress;
  std::string strTemp;
  size_t pos;
  size_t i = 3;

  do {
    pos = IP.find(".");

    if (pos != std::string::npos) {
      strTemp = IP.substr(0, pos);
      a[i] = atoi(strTemp.c_str());
      i--;
      IP.erase(0, pos + 1);
    } else {
      strTemp = IP;
      a[i] = atoi(strTemp.c_str());
      break;
    }

  } while (1);

  uint32_t nResult = (a[3] << 24) + (a[2] << 16) + (a[1] << 8) + a[0];
  return nResult;
}

uint64_t StringIdMaker::get_curr_ms() {
  struct timeval time_now;
  //windows and linux use the same function name, windows's defination as begining this file
#ifdef  WIN32
  gettimeofdayWin(&time_now, NULL); //  WIN32
#else
  gettimeofday(&time_now, NULL);	//LINUX
#endif

  uint64_t ms_time = time_now.tv_sec * 1000 + time_now.tv_usec / 1000;
  return ms_time;
}

void StringIdMaker::set_start_and_next_tm() {
  time_t tmNow = time(NULL);
  tm *ptmNow = localtime(&tmNow);
  tm mon_begin;
  mon_begin.tm_year = ptmNow->tm_year;
  mon_begin.tm_mon = ptmNow->tm_mon;
  mon_begin.tm_mday = 0;
  mon_begin.tm_hour = 0;
  mon_begin.tm_min = 0;
  mon_begin.tm_sec = 0;

  tm mon_next_begin;
  if (ptmNow->tm_mon == 12) {
    mon_next_begin.tm_year = ptmNow->tm_year + 1;
    mon_next_begin.tm_mon = 1;
  } else {
    mon_next_begin.tm_year = ptmNow->tm_year;
    mon_next_begin.tm_mon = ptmNow->tm_mon + 1;
  }
  mon_next_begin.tm_mday = 0;
  mon_next_begin.tm_hour = 0;
  mon_next_begin.tm_min = 0;
  mon_next_begin.tm_sec = 0;

  time_t mon_begin_tm = mktime(&mon_begin);
  time_t mon_end_tm = mktime(&mon_next_begin);

  _start_tm = mon_begin_tm * 1000;
  _next_start_tm = mon_end_tm * 1000;
}

int StringIdMaker::atomic_incr(int id) {
  #ifdef WIN32
	  InterlockedIncrement((LONG*)&id);
  #else
    __sync_add_and_fetch(&id, 1);
  #endif
  return id;
}
std::string StringIdMaker::get_unique_id() {
  uint64_t now_time = get_curr_ms();

  if (now_time > _next_start_tm) {
    set_start_and_next_tm();
  }
  uint32_t tm_period = now_time - _start_tm;
  seqid = atomic_incr(seqid) & 0xFF;

  std::size_t prifix_len = 10;  // 10 = prefix len
  unsigned char *write_index = _buff + prifix_len;

  memcpy(write_index, &tm_period, 4);
  write_index += 4;

  memcpy(write_index, &seqid, 2);

  hexdump(_buff + prifix_len, (_0x_buff + (2 * prifix_len)), 6);
  _0x_buff[32] = '\0';
  return std::string(_0x_buff);
}

void StringIdMaker::hexdump(unsigned char *buffer, char *out_buff, unsigned long index) {
  for (unsigned long i = 0; i < index; i++) {
    sprintf(out_buff + 2 * i, "%02X ", buffer[i]);
  }
}
}
