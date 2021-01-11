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
#ifndef ROCKETMQ_EXAMPLE_COMMON_H_
#define ROCKETMQ_EXAMPLE_COMMON_H_

#include <atomic>
#include <chrono>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#ifndef WIN32
#include <unistd.h>
#else
#include "ArgHelper.h"
#endif

#include "PullResult.h"

static std::atomic<int> g_msg_count(1);

class RocketmqSendAndConsumerArgs {
 public:
  RocketmqSendAndConsumerArgs()
      : body("msgbody for test"),
        thread_count(1),
        broadcasting(false),
        selectUnactiveBroker(false),
        isAutoDeleteSendCallback(true),
        retrytimes(1),
        printMoreInfo(false) {}

 public:
  std::string namesrv;
  std::string groupname;
  std::string topic;
  std::string body;
  int thread_count;
  bool broadcasting;
  bool selectUnactiveBroker;  // default select active broker
  bool isAutoDeleteSendCallback;
  int retrytimes;  // default retry 1 times;
  bool printMoreInfo;
};

class TpsReportService {
 public:
  TpsReportService() : tps_interval_(1), quit_flag_(false), tps_count_(0), all_count_(0) {}

  void start() {
    if (tps_thread_ == nullptr) {
      tps_thread_.reset(new std::thread(std::bind(&TpsReportService::TpsReport, this)));
    }
  }

  ~TpsReportService() {
    quit_flag_.store(true);

    if (tps_thread_ == nullptr) {
      std::cout << "tps_thread_ is null" << std::endl;
      return;
    }

    if (tps_thread_->joinable()) {
      tps_thread_->join();
    }
  }

  void Increment() { ++tps_count_; }

  void TpsReport() {
    while (!quit_flag_.load()) {
      std::this_thread::sleep_for(tps_interval_);
      auto tps = tps_count_.exchange(0);
      all_count_ += tps;
      std::cout << "all: " << all_count_ << ", tps: " << tps << std::endl;
    }
  }

 private:
  std::chrono::seconds tps_interval_;
  std::shared_ptr<std::thread> tps_thread_;
  std::atomic<bool> quit_flag_;
  std::atomic<long> tps_count_;
  uint64_t all_count_;
};

/*
static void PrintResult(rocketmq::SendResult* result) {
  std::cout << "sendresult = " << result->send_status()
            << ", msgid = " << result->getMsgId()
            << ", queueOffset = " << result->getQueueOffset() << ","
            << result->getMessageQueue().toString() << endl;
}
*/

void PrintPullResult(rocketmq::PullResult* result) {
  std::cout << result->toString() << std::endl;
  if (result->pull_status() == rocketmq::FOUND) {
    std::cout << result->toString() << std::endl;
    for (auto msg : result->msg_found_list()) {
      std::cout << "=======================================================" << std::endl
                << msg->toString() << std::endl;
    }
  }
}

static void PrintRocketmqSendAndConsumerArgs(const RocketmqSendAndConsumerArgs& info) {
  std::cout << "nameserver: " << info.namesrv << std::endl
            << "topic: " << info.topic << std::endl
            << "groupname: " << info.groupname << std::endl
            << "produce content: " << info.body << std::endl
            << "msg count: " << g_msg_count.load() << std::endl
            << "thread count: " << info.thread_count << std::endl;
}

static void help() {
  std::cout << "need option, like follow:\n"
               "-n nameserver addr, if not set -n, no namesrv will be got\n"
               "-g groupname\n"
               "-t msg topic\n"
               "-m messagecout(default value: 1)\n"
               "-c content(default value: \"msgbody for test\")\n"
               "-b (BROADCASTING model, default value: CLUSTER)\n"
               "-r setup retry times(default value: 1)\n"
               "-u select active broker to send msg(default value: false)\n"
               "-d use AutoDeleteSendcallback by cpp client(defalut value: false)\n"
               "-T thread count of send msg or consume msg(defalut value: 1)\n"
               "-v print more details information\n";
}

static bool ParseArgs(int argc, char* argv[], RocketmqSendAndConsumerArgs* info) {
#ifndef WIN32
  int ch;
  while ((ch = getopt(argc, argv, "n:g:t:m:c:br:uT:vh")) != -1) {
    switch (ch) {
      case 'n':
        info->namesrv.insert(0, optarg);
        break;
      case 'g':
        info->groupname.insert(0, optarg);
        break;
      case 't':
        info->topic.insert(0, optarg);
        break;
      case 'm':
        g_msg_count.store(atoi(optarg));
        break;
      case 'c':
        info->body.insert(0, optarg);
        break;
      case 'b':
        info->broadcasting = true;
        break;
      case 'r':
        info->retrytimes = atoi(optarg);
        break;
      case 'u':
        info->selectUnactiveBroker = true;
        break;
      case 'T':
        info->thread_count = atoi(optarg);
        break;
      case 'v':
        info->printMoreInfo = true;
        break;
      case 'h':
        help();
        return false;
      default:
        help();
        return false;
    }
  }
#else
  rocketmq::Arg_helper arg_help(argc, argv);
  info->namesrv = arg_help.get_option_value("-n");
  info->groupname = arg_help.get_option_value("-g");
  info->topic = arg_help.get_option_value("-t");
  info->broadcasting = atoi(arg_help.get_option_value("-b").c_str());
  string msgContent(arg_help.get_option_value("-c"));
  if (!msgContent.empty()) {
    info->body = msgContent;
  }
  int retrytimes = atoi(arg_help.get_option_value("-r").c_str());
  if (retrytimes > 0) {
    info->retrytimes = retrytimes;
  }
  info->selectUnactiveBroker = atoi(arg_help.get_option_value("-u").c_str());
  int thread_count = atoi(arg_help.get_option_value("-T").c_str());
  if (thread_count > 0) {
    info->thread_count = thread_count;
  }
  info->printMoreInfo = atoi(arg_help.get_option_value("-v").c_str());
  g_msg_count = atoi(arg_help.get_option_value("-m").c_str());
#endif
  if (info->groupname.empty() || info->topic.empty() || info->namesrv.empty()) {
    std::cout << "please use -g to setup groupname and -t setup topic \n";
    help();
    return false;
  }
  return true;
}

#endif  // ROCKETMQ_EXAMPLE_COMMON_H_
