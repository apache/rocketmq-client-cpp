add_executable(logger_test LoggerTest.cpp)
target_link_libraries(logger_test
        PRIVATE gtest_main
        PRIVATE logger_library
        PRIVATE spdlog::spdlog)