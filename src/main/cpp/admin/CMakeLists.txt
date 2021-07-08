add_library(admin_obj OBJECT)

target_include_directories(admin_obj
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_sources(admin_obj
        PRIVATE
            AdminServiceImpl.cpp
            AdminServerImpl.cpp
            AdminFacade.cpp)

target_link_libraries(admin_obj
        PUBLIC
            rocketmq_interface
            spdlog::spdlog_header_only
            mq_proto_library
            grpc++)

add_library(admin_library
        STATIC
            $<TARGET_OBJECTS:admin_obj>)
target_include_directories(admin_library
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(admin_library
        PUBLIC
            mq_proto_library
            grpc++
            rocketmq_interface
            spdlog::spdlog_header_only)


add_executable(admin_client AdminClient.cpp)
target_include_directories(admin_client
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(admin_client
        PRIVATE
            grpc++
            mq_proto_library
            rocketmq_interface
            $<$<PLATFORM_ID:Linux>:-static-libgcc>
            $<$<PLATFORM_ID:Linux>:-static-libstdc++>)