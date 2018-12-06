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
// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "big_endian.h"
#include <cstdlib>
#include <cstring>

namespace rocketmq {

BigEndianReader::BigEndianReader(const char* buf, size_t len)
    : ptr_(buf), end_(ptr_ + len) {}

bool BigEndianReader::Skip(size_t len) {
  if (ptr_ + len > end_) return false;
  ptr_ += len;
  return true;
}

bool BigEndianReader::ReadBytes(void* out, size_t len) {
  if (ptr_ + len > end_) return false;
  memcpy(out, ptr_, len);
  ptr_ += len;
  return true;
}

template <typename T>
bool BigEndianReader::Read(T* value) {
  if (ptr_ + sizeof(T) > end_) return false;
  ReadBigEndian<T>(ptr_, value);
  ptr_ += sizeof(T);
  return true;
}

bool BigEndianReader::ReadU8(uint8_t* value) { return Read(value); }

bool BigEndianReader::ReadU16(uint16_t* value) { return Read(value); }

bool BigEndianReader::ReadU32(uint32_t* value) { return Read(value); }

bool BigEndianReader::ReadU64(uint64_t* value) { return Read(value); }

BigEndianWriter::BigEndianWriter(char* buf, size_t len)
    : ptr_(buf), end_(ptr_ + len) {}

bool BigEndianWriter::Skip(size_t len) {
  if (ptr_ + len > end_) return false;
  ptr_ += len;
  return true;
}

bool BigEndianWriter::WriteBytes(const void* buf, size_t len) {
  if (ptr_ + len > end_) return false;
  memcpy(ptr_, buf, len);
  ptr_ += len;
  return true;
}

template <typename T>
bool BigEndianWriter::Write(T value) {
  if (ptr_ + sizeof(T) > end_) return false;
  WriteBigEndian<T>(ptr_, value);
  ptr_ += sizeof(T);
  return true;
}

bool BigEndianWriter::WriteU8(uint8_t value) { return Write(value); }

bool BigEndianWriter::WriteU16(uint16_t value) { return Write(value); }

bool BigEndianWriter::WriteU32(uint32_t value) { return Write(value); }

bool BigEndianWriter::WriteU64(uint64_t value) { return Write(value); }

}  // namespace base
