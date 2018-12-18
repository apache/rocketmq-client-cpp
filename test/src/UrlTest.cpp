/*
 * UrlTest.cpp
 *
 *  Created on: 2018年12月15日
 *      Author: laohu
 */

#include "url.h"
#include "TopicConfig.h"
#include "gtest/gtest.h"
#include "gmock/gmock.h"

using namespace std;
using ::testing::InitGoogleTest;
using ::testing::InitGoogleMock;
using testing::Return;
using rocketmq::Url;
using rocketmq::TopicConfig;

class MockTopicConfig : public  TopicConfig{
public:
	MOCK_METHOD0(getReadQueueNums , int());
};


TEST(Url, Url) {
	Url url_s("172.17.0.2:9876");
	EXPECT_EQ(url_s.protocol_ , "172.17.0.2:9876");

	Url url_z("https://www.aliyun.com/RocketMQ?5.0");
	EXPECT_EQ(url_z.protocol_ , "https");
	EXPECT_EQ(url_z.host_ , "www.aliyun.com");
	EXPECT_EQ(url_z.port_ , "80");
	EXPECT_EQ(url_z.path_ , "/RocketMQ");
	EXPECT_EQ(url_z.query_ , "5.0");

	Url url_path("https://www.aliyun.com:9876/RocketMQ?5.0");
	EXPECT_EQ(url_path.port_ , "9876");
	MockTopicConfig topicConfig;
	EXPECT_CALL(topicConfig , getReadQueueNums()).WillRepeatedly(Return(-1));
	int nums = topicConfig.getReadQueueNums();
	cout << nums << endl;

}

int main(int argc, char* argv[]) {
	InitGoogleMock(&argc, argv);

	testing::GTEST_FLAG(filter) = "Url.Url";
	return RUN_ALL_TESTS();
}
