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
#ifndef ROCKETMQ_COMMON_BYTEORDER_H_
#define ROCKETMQ_COMMON_BYTEORDER_H_

#include <cstdlib>  // std::memcpy

#include <type_traits>  // std::enable_if, std::is_integral, std::make_unsigned, std::add_pointer

#include "RocketMQClient.h"

namespace rocketmq {

enum ByteOrder { BO_BIG_ENDIAN, BO_LITTLE_ENDIAN };

/**
 * Contains static methods for converting the byte order between different endiannesses.
 */
class ByteOrderUtil {
 public:
  static constexpr ByteOrder native_order() {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__  // __BYTE_ORDER__ is defined by GCC
    return ByteOrder::BO_LITTLE_ENDIAN;
#else
    return ByteOrder::BO_BIG_ENDIAN;
#endif
  }

  /** Returns true if the current CPU is big-endian. */
  static constexpr bool isBigEndian() { return native_order() == ByteOrder::BO_BIG_ENDIAN; }

  //==============================================================================

  template <typename T, typename F, typename std::enable_if<sizeof(T) == sizeof(F), int>::type = 0>
  static inline T ReinterpretRawBits(F value) {
    return *reinterpret_cast<T*>(&value);
  }

  static inline uint8_t swap(uint8_t n) { return n; }

  /** Swaps the upper and lower bytes of a 16-bit integer. */
  static inline uint16_t swap(uint16_t n) { return static_cast<uint16_t>((n << 8) | (n >> 8)); }

  /** Reverses the order of the 4 bytes in a 32-bit integer. */
  static inline uint32_t swap(uint32_t n) {
    return (n << 24) | (n >> 24) | ((n & 0x0000ff00) << 8) | ((n & 0x00ff0000) >> 8);
  }

  /** Reverses the order of the 8 bytes in a 64-bit integer. */
  static inline uint64_t swap(uint64_t value) {
    return (((uint64_t)swap((uint32_t)value)) << 32) | swap((uint32_t)(value >> 32));
  }

  //==============================================================================

  /** convert integer to little-endian */
  template <typename T, typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
  static inline typename std::make_unsigned<T>::type NorminalLittleEndian(T value) {
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    return swap(static_cast<typename std::make_unsigned<T>::type>(value));
#else
    return static_cast<typename std::make_unsigned<T>::type>(value);
#endif
  }

  /** convert integer to big-endian */
  template <typename T, typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
  static inline typename std::make_unsigned<T>::type NorminalBigEndian(T value) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return swap(static_cast<typename std::make_unsigned<T>::type>(value));
#else
    return static_cast<typename std::make_unsigned<T>::type>(value);
#endif
  }

  //==============================================================================

  template <typename T, int Enable = 0>
  static inline T Read(const char* bytes) {
    T value;
    std::memcpy(&value, bytes, sizeof(T));
    return value;
  }

  template <typename T, typename std::enable_if<sizeof(T) <= 8, int>::value = 0>
  static inline T Read(const char* bytes) {
    T value;
    for (size_t i = 0; i < sizeof(T); i++) {
      ((char*)&value)[i] = bytes[i];
    }
    return value;
  }

  template <typename T, int Enable = 0>
  static inline void Read(T* value, const char* bytes) {
    std::memcpy(value, bytes, sizeof(T));
  }

  template <typename T, typename std::enable_if<sizeof(T) <= 8, int>::value = 0>
  static inline void Read(T* value, const char* bytes) {
    for (size_t i = 0; i < sizeof(T); i++) {
      ((char*)value)[i] = bytes[i];
    }
  }

  //==============================================================================

  template <typename T, int Enable = 0>
  static inline void Write(char* bytes, T value) {
    std::memcpy(bytes, &value, sizeof(T));
  }

  template <typename T, typename std::enable_if<sizeof(T) <= 8, int>::value = 0>
  static inline void Write(char* bytes, T value) {
    for (size_t i = 0; i < sizeof(T); i++) {
      bytes[i] = ((char*)&value)[i];
    }
  }

  //==============================================================================

  template <typename T, typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
  static inline T ReadLittleEndian(const char* bytes) {
    auto value = Read<T>(bytes);
    return NorminalLittleEndian(value);
  }

  template <typename T, typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
  static inline T ReadBigEndian(const char* bytes) {
    auto value = Read<T>(bytes);
    return NorminalBigEndian(value);
  }

  template <typename T, typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
  static inline T Read(const char* bytes, bool big_endian) {
    return big_endian ? ReadBigEndian<T>(bytes) : ReadLittleEndian<T>(bytes);
  }

  //==============================================================================

  template <typename T, typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
  static inline void WriteLittleEndian(char* bytes, T value) {
    value = NorminalLittleEndian(value);
    Write(bytes, value);
  }

  template <typename T, typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
  static inline void WriteBigEndian(char* bytes, T value) {
    value = NorminalBigEndian(value);
    Write(bytes, value);
  }

  template <typename T, typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
  static inline void Write(char* bytes, T value, bool big_endian) {
    if (big_endian) {
      WriteBigEndian(bytes, value);
    } else {
      WriteLittleEndian(bytes, value);
    }
  }
};

}  // namespace rocketmq

#endif  // ROCKETMQ_COMMON_BYTEORDER_H_
