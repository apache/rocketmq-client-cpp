add_executable(countdown_latch_test CountdownLatchTest.cpp)
target_link_libraries(countdown_latch_test
        PRIVATE
            gtest_main
            countdown_latch_library
            spdlog)
add_test(ut_countdown_latch_test countdown_latch_test)

add_executable(scheduler_test SchedulerTest.cpp)
target_link_libraries(scheduler_test
        PRIVATE
            absl::synchronization
            base_library
            gtest_main
            scheduler_library)
add_test(ut_scheduler_test scheduler_test)

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