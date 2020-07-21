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
#include "ArgHelper.h"

#include "UtilAll.h"

namespace rocketmq {

ArgHelper::ArgHelper(int argc, char* argv[]) {
  for (int i = 0; i < argc; i++) {
    args_.push_back(argv[i]);
  }
}

ArgHelper::ArgHelper(std::string arg_str_) {
  std::vector<std::string> v;
  UtilAll::Split(v, arg_str_, " ");
  args_.insert(args_.end(), v.begin(), v.end());
}

std::string ArgHelper::get_option(int idx_) const {
  if ((size_t)idx_ >= args_.size()) {
    return "";
  }
  return args_[idx_];
}

bool ArgHelper::is_enable_option(std::string opt_) const {
  for (size_t i = 0; i < args_.size(); ++i) {
    if (opt_ == args_[i]) {
      return true;
    }
  }
  return false;
}

std::string ArgHelper::get_option_value(std::string opt_) const {
  std::string ret = "";
  for (size_t i = 0; i < args_.size(); ++i) {
    if (opt_ == args_[i]) {
      size_t value_idx = ++i;
      if (value_idx >= args_.size()) {
        return ret;
      }
      ret = args_[value_idx];
      return ret;
    }
  }
  return ret;
}

}  // namespace rocketmq
