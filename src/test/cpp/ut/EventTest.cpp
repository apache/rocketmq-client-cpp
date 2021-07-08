#include "uv.h"
#include "gtest/gtest.h"
#include <iostream>

class EventTest : public testing::Test {
public:
  static void sample_callback(uv_timer_t* handle) {
    if (count++ > 3) {
      uv_timer_stop(handle);
    }
  }

  void SetUp() {}

  void TearDown() {}

protected:
  static int count;
};
int EventTest::count = 0;

TEST_F(EventTest, testEvent) {
  uv_loop_t* loop = uv_default_loop();
  uv_timer_t timer;
  uv_timer_init(loop, &timer);
  uv_timer_start(&timer, &EventTest::sample_callback, 10, 20);
  uv_run(loop, UV_RUN_DEFAULT);
  uv_loop_close(loop);
  EXPECT_TRUE(EventTest::count >= 3);
}

TEST_F(EventTest, test_loop_close) {
  uv_loop_t* loop = uv_default_loop();
  if (uv_loop_alive(loop)) {
    uv_loop_close(loop);
  }
}