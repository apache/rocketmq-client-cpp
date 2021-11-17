#include "QueryConsumerOffsetResponseHeader.h"

#include "gtest/gtest.h"

ROCKETMQ_NAMESPACE_BEGIN

TEST(QueryConsumerOffsetResponseHeaderTest, testDecode) {
  std::string json = R"({"offset": 1})";

  google::protobuf::Value root;
  auto status = google::protobuf::util::JsonStringToMessage(json, &root);

  ASSERT_TRUE(status.ok());

  auto header = QueryConsumerOffsetResponseHeader::decode(root);
  ASSERT_EQ(1, header->offset_);
}

ROCKETMQ_NAMESPACE_END