add_executable(countdown_latch_test CountdownLatchTest.cpp)
target_link_libraries(countdown_latch_test
        PRIVATE
        gtest_main
        countdown_latch_library
        spdlog)
add_test(ut_countdown_latch_test countdown_latch_test)