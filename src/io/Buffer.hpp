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
#ifndef ROCKETMQ_IO_BUFFER_HPP_
#define ROCKETMQ_IO_BUFFER_HPP_

#include <stdexcept>  // std::invalid_argument, std::runtime_error

#include "RocketMQClient.h"

#include "UtilAll.h"

namespace rocketmq {

template <typename A>
class Buffer {
 protected:
  Buffer(int32_t mark, int32_t pos, int32_t lim, int32_t cap) {
    if (cap < 0) {
      throw std::invalid_argument("Negative capacity: " + UtilAll::to_string(cap));
    }
    capacity_ = cap;
    limit(lim);
    position(pos);
    if (mark >= 0) {
      if (mark > pos) {
        throw std::invalid_argument("mark > position: (" + UtilAll::to_string(mark) + " > " + UtilAll::to_string(pos) +
                                    ")");
      }
      mark_ = mark;
    }
  }

 public:
  Buffer& position(int new_position) {
    if ((new_position > limit_) || (new_position < 0)) {
      throw std::invalid_argument("");
    }
    position_ = new_position;
    if (mark_ > position_) {
      mark_ = -1;
    }
    return *this;
  }

  Buffer& limit(int new_limit) {
    if ((new_limit > capacity_) || (new_limit < 0)) {
      throw std::invalid_argument("");
    }
    limit_ = new_limit;
    if (position_ > limit_) {
      position_ = limit_;
    }
    if (mark_ > limit_) {
      mark_ = -1;
    }
    return *this;
  }

  Buffer& mark() {
    mark_ = position_;
    return *this;
  }

  Buffer& reset() {
    int m = mark_;
    if (m < 0) {
      throw std::runtime_error("InvalidMarkException");
    }
    position_ = m;
    return *this;
  }

  Buffer& clear() {
    position_ = 0;
    limit_ = capacity_;
    mark_ = -1;
    return *this;
  }

  Buffer& flip() {
    limit_ = position_;
    position_ = 0;
    mark_ = -1;
    return *this;
  }

  Buffer& rewind() {
    position_ = 0;
    mark_ = -1;
    return *this;
  }

  inline int32_t remaining() const { return limit_ - position_; }
  inline bool hasRemaining() const { return position_ < limit_; }

  virtual bool isReadOnly() = 0;
  virtual bool hasArray() = 0;
  virtual A* array() = 0;
  virtual int32_t arrayOffset() = 0;

 protected:
  int32_t nextGetIndex() {
    if (position_ >= limit_) {
      throw std::runtime_error("BufferUnderflowException");
    }
    return position_++;
  }

  int32_t nextGetIndex(int32_t nb) {
    if (limit_ - position_ < nb) {
      throw std::runtime_error("BufferUnderflowException");
    }
    int32_t p = position_;
    position_ += nb;
    return p;
  }

  int32_t nextPutIndex() {
    if (position_ >= limit_) {
      throw std::runtime_error("BufferOverflowException");
    }
    return position_++;
  }

  int32_t nextPutIndex(int32_t nb) {
    if (limit_ - position_ < nb) {
      throw std::runtime_error("BufferOverflowException");
    }
    int32_t p = position_;
    position_ += nb;
    return p;
  }

  int32_t checkIndex(int32_t i) const {
    if ((i < 0) || (i >= limit_)) {
      throw std::runtime_error("IndexOutOfBoundsException");
    }
    return i;
  }

  int32_t checkIndex(int32_t i, int32_t nb) const {
    if ((i < 0) || (nb > limit_ - i)) {
      throw std::runtime_error("IndexOutOfBoundsException");
    }
    return i;
  }

  void truncate() {
    mark_ = -1;
    position_ = 0;
    limit_ = 0;
    capacity_ = 0;
  }

  void discardMark() { mark_ = -1; }

  static void checkBounds(int32_t off, int32_t len, int32_t size) {
    if ((off | len | (off + len) | (size - (off + len))) < 0) {
      throw std::runtime_error("IndexOutOfBoundsException");
    }
  }

 public:
  inline int32_t mark_value() const { return mark_; }
  inline int32_t position() const { return position_; }
  inline int32_t limit() const { return limit_; }
  inline int32_t capacity() const { return capacity_; }

 private:
  // Invariants: mark <= position <= limit <= capacity
  int32_t mark_ = -1;
  int32_t position_ = 0;
  int32_t limit_;
  int32_t capacity_;
};

}  // namespace rocketmq

#endif  // ROCKETMQ_IO_BUFFER_HPP_
