add_executable(client_instance_test ClientInstanceTest.cpp)
target_link_libraries(client_instance_test
        PRIVATE
            client_library
            gtest_main)
add_test(it_client_instance client_instance_test)

add_executable(rpc_client_test RpcClientTest.cpp)
target_link_libraries(rpc_client_test
        PRIVATE
            client_library
            credentials_observable_mock
            rpc_client_mock
            gtest_main)
add_test(it_rpc_client_2 rpc_client_test)

add_executable(top_addressing_test TopAddressingTest.cpp)
target_link_libraries(top_addressing_test
        PRIVATE
            base_library
            gtest_main)
add_test(it_top_addressing top_addressing_test)


add_executable(topic_publish_info_test TopicPublishInfoTest.cpp)
target_link_libraries(topic_publish_info_test
        PRIVATE
            client_library
            credentials_observable_mock
            gtest_main)
add_test(it_topic_publish_info topic_publish_info_test)

add_executable(default_push_consumer_test DefaultMQPushConsumerIntegrationTest.cpp)
target_link_libraries(default_push_consumer_test
        PRIVATE
            rocketmq_library
            gtest_main)
add_test(it_default_push_consumer default_push_consumer_test)