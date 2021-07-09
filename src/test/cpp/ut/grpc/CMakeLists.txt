add_executable(grpc_thread_pool_test GrpcThreadPoolTest.cpp)
target_link_libraries(grpc_thread_pool_test
        PRIVATE
            grpc
            base_library
            gtest_main)
add_test(ut_grpc_thread_pool grpc_thread_pool_test)