add_library(rocketmq_mocks INTERFACE)
target_include_directories(rocketmq_mocks
        INTERFACE
            ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(rocketmq_mocks
        INTERFACE
            rocketmq_library
            gmock)