add_executable(admin_server_test AdminServerTest.cpp)
target_link_libraries(admin_server_test
        PRIVATE
            gtest_main
            admin_library
        )