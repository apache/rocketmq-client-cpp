add_library(client_mocks RpcClientMock.cpp)
target_include_directories(client_mocks
        INTERFACE
            ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(client_mocks
        PUBLIC
            gmock
            client_library
            mq_proto_library)