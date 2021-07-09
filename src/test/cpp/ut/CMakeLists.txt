add_executable(gtest_example GTestExample.cpp)
target_link_libraries(gtest_example gtest_main)
add_test(ut_gtest_example gtest_example)

add_executable(log_test LogTest.cpp)
target_link_libraries(log_test
        spdlog
        gtest_main)
add_test(ut_log_test log_test)

add_executable(event_test EventTest.cpp)
target_link_libraries(event_test
        gtest_main
        uv_a)

add_executable(zlib_test ZlibTest.cpp)
target_link_libraries(zlib_test
        PRIVATE
            gtest_main
            zlib)

add_executable(abseil_time_test AbseilTimeTest.cpp)
target_link_libraries(abseil_time_test
        PRIVATE
            gtest_main
            absl::time)

add_executable(hex_test HexTest.cpp)
target_link_libraries(hex_test
        PRIVATE
            absl::strings
            gtest_main)

add_subdirectory(base)
add_subdirectory(concurrent)
add_subdirectory(scheduler)
add_subdirectory(client)
add_subdirectory(grpc)
add_subdirectory(rocketmq)
add_subdirectory(logger)
add_subdirectory(admin)