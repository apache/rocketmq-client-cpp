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
#ifndef __BYTE_ORDER_H__
#define __BYTE_ORDER_H__

#include <RocketMQClient.h>

namespace rocketmq {

/**
 * Contains static methods for converting the byte order between different endiannesses.
 */
class ROCKETMQCLIENT_API ByteOrder {
 public:
  //==============================================================================
  /** Swaps the upper and lower bytes of a 16-bit integer. */
  static uint16_t swap(uint16_t value);

  /** Reverses the order of the 4 bytes in a 32-bit integer. */
  static uint32_t swap(uint32_t value);

  /** Reverses the order of the 8 bytes in a 64-bit integer. */
  static uint64_t swap(uint64_t value);

  //==============================================================================
  /** Swaps the byte order of a 16-bit int if the CPU is big-endian */
  static uint16_t swapIfBigEndian(uint16_t value);

  /** Swaps the byte order of a 32-bit int if the CPU is big-endian */
  static uint32_t swapIfBigEndian(uint32_t value);

  /** Swaps the byte order of a 64-bit int if the CPU is big-endian */
  static uint64_t swapIfBigEndian(uint64_t value);

  /** Swaps the byte order of a 16-bit int if the CPU is little-endian */
  static uint16_t swapIfLittleEndian(uint16_t value);

  /** Swaps the byte order of a 32-bit int if the CPU is little-endian */
  static uint32_t swapIfLittleEndian(uint32_t value);

  /** Swaps the byte order of a 64-bit int if the CPU is little-endian */
  static uint64_t swapIfLittleEndian(uint64_t value);

  //==============================================================================
  /** Turns 4 bytes into a little-endian integer. */
  static uint32_t littleEndianInt(const void* bytes);

  /** Turns 8 bytes into a little-endian integer. */
  static uint64_t littleEndianInt64(const void* bytes);

  /** Turns 2 bytes into a little-endian integer. */
  static uint16_t littleEndianShort(const void* bytes);

  /** Turns 4 bytes into a big-endian integer. */
  static uint32_t bigEndianInt(const void* bytes);

  /** Turns 8 bytes into a big-endian integer. */
  static uint64_t bigEndianInt64(const void* bytes);

  /** Turns 2 bytes into a big-endian integer. */
  static uint16_t bigEndianShort(const void* bytes);

  //==============================================================================
  /** Converts 3 little-endian bytes into a signed 24-bit value (which is
   * sign-extended to 32 bits). */
  static int littleEndian24Bit(const void* bytes);

  /** Converts 3 big-endian bytes into a signed 24-bit value (which is
   * sign-extended to 32 bits). */
  static int bigEndian24Bit(const void* bytes);

  /** Copies a 24-bit number to 3 little-endian bytes. */
  static void littleEndian24BitToChars(int value, void* destBytes);

  /** Copies a 24-bit number to 3 big-endian bytes. */
  static void bigEndian24BitToChars(int value, void* destBytes);

  //==============================================================================
  /** Returns true if the current CPU is big-endian. */
  static bool isBigEndian();
};

//==============================================================================

inline uint16_t ByteOrder::swap(uint16_t n) {
  return static_cast<uint16_t>((n << 8) | (n >> 8));
}

inline uint32_t ByteOrder::swap(uint32_t n) {
  return (n << 24) | (n >> 24) | ((n & 0xff00) << 8) | ((n & 0xff0000) >> 8);
}

inline uint64_t ByteOrder::swap(uint64_t value) {
  return (((uint64_t)swap((uint32_t)value)) << 32) | swap((uint32_t)(value >> 32));
}

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__  //__BYTE_ORDER__ is defined by GCC
inline uint16_t ByteOrder::swapIfBigEndian(const uint16_t v) {
  return v;
}

inline uint32_t ByteOrder::swapIfBigEndian(const uint32_t v) {
  return v;
}

inline uint64_t ByteOrder::swapIfBigEndian(const uint64_t v) {
  return v;
}

inline uint16_t ByteOrder::swapIfLittleEndian(const uint16_t v) {
  return swap(v);
}

inline uint32_t ByteOrder::swapIfLittleEndian(const uint32_t v) {
  return swap(v);
}

inline uint64_t ByteOrder::swapIfLittleEndian(const uint64_t v) {
  return swap(v);
}

inline uint32_t ByteOrder::littleEndianInt(const void* const bytes) {
  return *static_cast<const uint32_t*>(bytes);
}

inline uint64_t ByteOrder::littleEndianInt64(const void* const bytes) {
  return *static_cast<const uint64_t*>(bytes);
}

inline uint16_t ByteOrder::littleEndianShort(const void* const bytes) {
  return *static_cast<const uint16_t*>(bytes);
}

inline uint32_t ByteOrder::bigEndianInt(const void* const bytes) {
  return swap(*static_cast<const uint32_t*>(bytes));
}

inline uint64_t ByteOrder::bigEndianInt64(const void* const bytes) {
  return swap(*static_cast<const uint64_t*>(bytes));
}

inline uint16_t ByteOrder::bigEndianShort(const void* const bytes) {
  return swap(*static_cast<const uint16_t*>(bytes));
}

inline bool ByteOrder::isBigEndian() {
  return false;
}
#else
inline uint16_t ByteOrder::swapIfBigEndian(const uint16_t v) {
  return swap(v);
}

inline uint32_t ByteOrder::swapIfBigEndian(const uint32_t v) {
  return swap(v);
}

inline uint64_t ByteOrder::swapIfBigEndian(const uint64_t v) {
  return swap(v);
}

inline uint16_t ByteOrder::swapIfLittleEndian(const uint16_t v) {
  return v;
}

inline uint32_t ByteOrder::swapIfLittleEndian(const uint32_t v) {
  return v;
}

inline uint64_t ByteOrder::swapIfLittleEndian(const uint64_t v) {
  return v;
}

inline uint32_t ByteOrder::littleEndianInt(const void* const bytes) {
  return swap(*static_cast<const uint32_t*>(bytes));
}

inline uint64_t ByteOrder::littleEndianInt64(const void* const bytes) {
  return swap(*static_cast<const uint64_t*>(bytes));
}

inline uint16_t ByteOrder::littleEndianShort(const void* const bytes) {
  return swap(*static_cast<const uint16_t*>(bytes));
}

inline uint32_t ByteOrder::bigEndianInt(const void* const bytes) {
  return *static_cast<const uint32_t*>(bytes);
}

inline uint64_t ByteOrder::bigEndianInt64(const void* const bytes) {
  return *static_cast<const uint64_t*>(bytes);
}

inline uint16_t ByteOrder::bigEndianShort(const void* const bytes) {
  return *static_cast<const uint16_t*>(bytes);
}

inline bool ByteOrder::isBigEndian() {
  return true;
}
#endif

inline int ByteOrder::littleEndian24Bit(const void* const bytes) {
  return (((int)static_cast<const int8_t*>(bytes)[2]) << 16) | (((int)static_cast<const uint8_t*>(bytes)[1]) << 8) |
         ((int)static_cast<const uint8_t*>(bytes)[0]);
}

inline int ByteOrder::bigEndian24Bit(const void* const bytes) {
  return (((int)static_cast<const int8_t*>(bytes)[0]) << 16) | (((int)static_cast<const uint8_t*>(bytes)[1]) << 8) |
         ((int)static_cast<const uint8_t*>(bytes)[2]);
}

inline void ByteOrder::littleEndian24BitToChars(const int value, void* const destBytes) {
  static_cast<uint8_t*>(destBytes)[0] = (uint8_t)value;
  static_cast<uint8_t*>(destBytes)[1] = (uint8_t)(value >> 8);
  static_cast<uint8_t*>(destBytes)[2] = (uint8_t)(value >> 16);
}

inline void ByteOrder::bigEndian24BitToChars(const int value, void* const destBytes) {
  static_cast<uint8_t*>(destBytes)[0] = (uint8_t)(value >> 16);
  static_cast<uint8_t*>(destBytes)[1] = (uint8_t)(value >> 8);
  static_cast<uint8_t*>(destBytes)[2] = (uint8_t)value;
}

}  // namespace rocketmq

#endif  // __BYTE_ORDER_H__
