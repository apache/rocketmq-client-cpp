add_executable(mix_all_test MixAllTest.cpp)
target_link_libraries(mix_all_test
        PRIVATE
            base_library
            gtest_main)

add_executable(util_all_test UtilAllTest.cpp)
target_link_libraries(util_all_test
        PRIVATE
            base_library
            spdlog::spdlog
            absl::base
            gtest_main)

add_executable(rate_limiter_test RateLimiterTest.cpp)
target_link_libraries(rate_limiter_test PRIVATE
            gtest_main
            base_library)

add_executable(histogram_test HistogramTest.cpp)
target_link_libraries(histogram_test
        PRIVATE
            gtest_main
            base_library)
add_test(test_histogram histogram_test)


add_executable(unique_id_generator_test UniqueIdGeneratorTest.cpp)
target_link_libraries(unique_id_generator_test
        PRIVATE
            gtest_main
            base_library)
add_test(ut_unique_id_generator unique_id_generator_test)

add_executable(ghttp_client_test GHttpClientTest.cpp)
target_link_libraries(ghttp_client_test
        PRIVATE
            base_library
            gtest_main)