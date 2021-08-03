add_library(client_test_interface INTERFACE)
target_include_directories(client_test_interface
        INTERFACE
            ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(client_test_interface
        INTERFACE
            rpc_client_mock
            gtest_main)

add_executable(default_mq_producer_test DefaultMQProducerTest.cpp)
target_link_libraries(default_mq_producer_test
        PRIVATE
            client_test_interface
            rocketmq_library)
add_test(ut_default_mq_producer_test default_mq_producer_test)

add_executable(default_mq_push_consumer_test DefaultMQPushConsumerTest.cpp)
target_link_libraries(default_mq_push_consumer_test
        rocketmq_library
        client_test_interface)

add_executable(executor_test ExecutorTest.cpp)
target_link_libraries(executor_test
        PRIVATE
            rocketmq_interface
            gtest_main)

add_executable(message_test MessageTest.cpp)
target_link_libraries(message_test
        PRIVATE
            rocketmq_library
            gtest_main)

add_executable(process_queue_test ProcessQueueTest.cpp)
target_link_libraries(process_queue_test
        PRIVATE
            gtest_main
            base_mocks
            client_mocks
            rocketmq_mocks
            rocketmq_library)

add_executable(ut_credentials_provider CredentialsProviderTest.cpp)
target_link_libraries(ut_credentials_provider
        gtest_main
        rocketmq_library)

add_executable(ut_send_callbacks SendCallbacksTest.cpp)
target_link_libraries(ut_send_callbacks
        PRIVATE
            gtest_main
            rocketmq_library)

add_executable(consume_message_service ConsumeStandardMessageServiceTest.cpp)
target_link_libraries(consume_message_service
        PRIVATE
            base_mocks
            gtest_main
            rocketmq_library
            rocketmq_mocks)

add_executable(filter_expression_test FilterExpressionTest.cpp)
target_link_libraries(filter_expression_test
        PRIVATE
            gtest_main
            rocketmq_library)

add_executable(message_group_queue_selector_test MessageGroupQueueSelectorTest.cpp)
target_link_libraries(message_group_queue_selector_test
        PRIVATE
            gtest_main
            rocketmq_library)

add_executable(sts_credentials_provider_test StsCredentialsProviderImplTest.cpp)
target_link_libraries(sts_credentials_provider_test
        PRIVATE
            base_mocks
            gtest_main
            rocketmq_library)

add_executable(consumer_test ConsumerTest.cpp)
target_link_libraries(consumer_test
        PRIVATE
            rocketmq_mocks
            gtest_main)

add_executable(await_pull_callback_test AwaitPullCallbackTest.cpp)
target_link_libraries(await_pull_callback_test
        PRIVATE
            rocketmq_library
            gtest_main)