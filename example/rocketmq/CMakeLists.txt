add_executable(example_producer ExampleProducer.cpp)
target_link_libraries(example_producer
        PRIVATE
            CURL::libcurl
            rocketmq
            $<$<PLATFORM_ID:Linux>:-static-libgcc>
            $<$<PLATFORM_ID:Linux>:-static-libstdc++>)

add_executable(example_async_producer ExampleAsyncProducer.cpp)
target_link_libraries(example_async_producer
        PRIVATE
            CURL::libcurl
            rocketmq
            $<$<PLATFORM_ID:Linux>:-static-libgcc>
            $<$<PLATFORM_ID:Linux>:-static-libstdc++>)


add_executable(example_push_consumer ExamplePushConsumer.cpp)
target_link_libraries(example_push_consumer
        PRIVATE
            CURL::libcurl
            rocketmq
            spdlog::spdlog_header_only
            $<$<PLATFORM_ID:Linux>:-static-libgcc>
            $<$<PLATFORM_ID:Linux>:-static-libstdc++>)


add_executable(example_broadcast_push_consumer ExampleBroadcastPushConsumer.cpp)
target_link_libraries(example_broadcast_push_consumer
        PRIVATE
        CURL::libcurl
        rocketmq
        spdlog::spdlog_header_only
        $<$<PLATFORM_ID:Linux>:-static-libgcc>
        $<$<PLATFORM_ID:Linux>:-static-libstdc++>)

add_executable(benchmark_push_consumer BenchmarkPushConsumer.cpp)
target_link_libraries(benchmark_push_consumer
        CURL::libcurl
        rocketmq)

add_executable(sql_producer SqlProducer.cpp)
target_link_libraries(sql_producer
        CURL::libcurl
        rocketmq)

add_executable(sql_consumer SqlConsumer.cpp)
target_link_libraries(sql_consumer
        CURL::libcurl
        rocketmq)

add_executable(consumer_with_custom_executor PushConsumerWithCustomExecutor.cpp)
target_link_libraries(consumer_with_custom_executor
        CURL::libcurl
        rocketmq)

add_executable(consumer_with_throttle PushConsumerWithThrottle.cpp)
target_link_libraries(consumer_with_throttle
        CURL::libcurl
        rocketmq)

add_executable(pull_consumer ExamplePullConsumer.cpp)
target_link_libraries(pull_consumer
        CURL::libcurl
        rocketmq)