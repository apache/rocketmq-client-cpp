add_library(semaphore_library
        Semaphore.cpp)
target_include_directories(semaphore_library
        INTERFACE
            ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(semaphore_library
        absl::synchronization)

add_library(client_test_interface INTERFACE)
target_include_directories(client_test_interface
        INTERFACE
            ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(client_test_interface
        INTERFACE
            rpc_client_mock
            gtest_main)

add_executable(semaphore_test SemaphoreTest.cpp)
target_link_libraries(semaphore_test
        gtest_main
        semaphore_library
        spdlog::spdlog)
add_test(ut_semaphore_test semaphore_test)

add_executable(default_mq_producer_test DefaultMQProducerTest.cpp)
target_link_libraries(default_mq_producer_test
        PRIVATE
            client_test_interface
            rocketmq_library
            semaphore_library)
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
        gtest_main
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