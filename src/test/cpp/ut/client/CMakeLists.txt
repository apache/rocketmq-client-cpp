if (ENABLE_TRACING)
#    add_executable(tracing_utility_test TracingUtilityTest.cpp)
#    target_link_libraries(tracing_utility_test
#            PRIVATE
#            client_library
#            gtest_main)
#    add_test(ut_tracing_utility_test tracing_utility_test)
endif ()

add_executable(tls_helper_test TlsHelperTest.cpp)
target_link_libraries(tls_helper_test
        PRIVATE
            client_library
            gtest_main)


add_executable(rpc_client_unit_test RpcClientTest.cpp)
target_link_libraries(rpc_client_unit_test
        PRIVATE
            rpc_client_mock
            client_library
            gtest_main)
add_test(ut_rpc_client rpc_client_unit_test)