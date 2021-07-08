#include "zlib.h"
#include "gtest/gtest.h"
#include <iostream>

TEST(ZlibTest, testDeflate) {

  uint32_t len = 1024 * 1024;
  auto data = new unsigned char[len];

  for (uint32_t i = 0; i < len; i++) {
    data[i] = static_cast<char>(i % 128);
  }

  z_stream stream;
  stream.zalloc = Z_NULL;
  stream.zfree = Z_NULL;
  stream.opaque = Z_NULL;
  stream.avail_in = len;
  stream.next_in = data;

  deflateInit(&stream, Z_BEST_COMPRESSION);
  uint32_t bound = deflateBound(&stream, len);
  auto compressed = new unsigned char[bound];
  stream.avail_out = bound;
  stream.next_out = compressed;

  int status = deflate(&stream, Z_FINISH);
  if (stream.msg) {
    std::cout << stream.msg << std::endl;
  }
  EXPECT_EQ(Z_STREAM_END, status);
  deflateEnd(&stream);

  z_stream inflate_stream;
  inflate_stream.zfree = Z_NULL;
  inflate_stream.zalloc = Z_NULL;
  inflate_stream.opaque = Z_NULL;

  inflate_stream.avail_in = stream.total_out;
  inflate_stream.next_in = compressed;

  uint32_t buffer_len = 1024 * 4;
  auto buffer = new unsigned char[buffer_len];

  inflateInit(&inflate_stream);
  inflate_stream.next_out = buffer;
  inflate_stream.avail_out = buffer_len;

  std::vector<unsigned char> raw;
  while (true) {
    status = inflate(&inflate_stream, Z_SYNC_FLUSH);

    if (Z_STREAM_ERROR == status) {
      if (inflate_stream.msg) {
        std::cerr << inflate_stream.msg << std::endl;
      }
      break;
    }
    raw.reserve(raw.size() + buffer_len - inflate_stream.avail_out);
    std::copy(&buffer[0], &buffer[buffer_len - inflate_stream.avail_out], std::back_inserter(raw));
    inflate_stream.avail_out = buffer_len;
    inflate_stream.next_out = buffer;

    if (Z_STREAM_END == status) {
      std::cout << "Inflation done! Total in: " << inflate_stream.total_in << ", Total out:" << inflate_stream.total_out
                << std::endl;
      break;
    }

    if (Z_OK == status) {
      std::cout << "Some data are inflated and flushed. Available in: " << inflate_stream.avail_in
                << ", Total out:" << inflate_stream.total_out << std::endl;
    }
  }

  EXPECT_EQ(std::vector<unsigned char>(data, data + len), raw);

  delete[] data;
  delete[] compressed;
  delete[] buffer;
}