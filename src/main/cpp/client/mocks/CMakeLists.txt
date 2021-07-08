add_library(rpc_client_mock RpcClientMock.cpp)
target_include_directories(rpc_client_mock
        INTERFACE
            ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(rpc_client_mock
        gmock
        client_library
        mq_proto_library)

add_library(credentials_observable_mock INTERFACE)
target_include_directories(credentials_observable_mock
        INTERFACE
            ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(credentials_observable_mock
        INTERFACE
            gmock
            client_library)