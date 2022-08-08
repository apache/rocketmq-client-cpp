#include "ons/ONSCallback.h"
#include "ons/ONSFactory.h"
#include "rocketmq/Logger.h"
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <string>

using namespace std;
using namespace ons;

std::mutex m1;
std::mutex m2;
std::condition_variable cv;

class MyCallback : public SendCallbackONS {
public:
  void onSuccess(SendResultONS& send_result) override {
    std::lock_guard<std::mutex> lg(m2);
    success_num++;
    std::cout << "send success, message_id: " << send_result.getMessageId() << ", total: " << success_num << std::endl;
    if (success_num + failed_num == total) {
      cv.notify_all();
    }
  }

  void onException(ONSClientException& e) override {
    std::lock_guard<std::mutex> lg(m2);
    failed_num++;
    std::cout << "send failure, total: " << failed_num << std::endl;
    std::cout << e.what() << std::endl;
    if (success_num + failed_num == total) {
      cv.notify_all();
    }
  }

  static int success_num;
  static int failed_num;
  static int total;
};

int MyCallback::success_num = 0;
int MyCallback::failed_num = 0;
int MyCallback::total = 256;

int main(int argc, char* argv[]) {
  rocketmq::Logger& logger = rocketmq::getLogger();
  logger.setLevel(rocketmq::Level::Debug);
  logger.init();

  std::cout << "=======Before send message=======" << std::endl;
  ONSFactoryProperty factoryInfo;

  /*
    factoryInfo.setFactoryProperty(ONSFactoryProperty::GroupId, "Your-GroupId");
    factoryInfo.setFactoryProperty(ONSFactoryProperty::AccessKey, "Your-Access-Key");
    factoryInfo.setFactoryProperty(ONSFactoryProperty::SecretKey, "Your-Secret-Key");
    factoryInfo.setFactoryProperty(ONSFactoryProperty::NAMESRV_ADDR, "Your-Access-Point-URL");
  */

  Producer* producer = nullptr;
  producer = ONSFactory::getInstance()->createProducer(factoryInfo);
  producer->start();
  Message msg("cpp_sdk_standard", "Your Tag", "Your Key", "This message body.");

  MyCallback m_callback;
  for (int i = 0; i < MyCallback::total; ++i) {
    msg.setTag(std::to_string(i));
    producer->sendAsync(msg, &m_callback);
  }

  {
    std::unique_lock<std::mutex> lk(m1);
    cv.wait(lk);
  }

  producer->shutdown();
  std::cout << "=======After sending messages=======" << std::endl;

  return 0;
}