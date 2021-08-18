add_subdirectory(base)
add_subdirectory(log)
add_subdirectory(admin)
add_subdirectory(concurrent)
add_subdirectory(scheduler)
add_subdirectory(client)
add_subdirectory(rocketmq)

add_library(rocketmq_static
        STATIC
            $<TARGET_OBJECTS:base_obj>
            $<TARGET_OBJECTS:admin_obj>
            $<TARGET_OBJECTS:scheduler_obj>
            $<TARGET_OBJECTS:client_obj>
            $<TARGET_OBJECTS:logger_obj>
            $<TARGET_OBJECTS:countdown_latch_obj>
            $<TARGET_OBJECTS:rocketmq_obj>)
target_link_libraries(rocketmq_static
        PUBLIC
            absl::flat_hash_set
            absl::flat_hash_map
            absl::random_random
            absl::synchronization
            absl::strings
            fmt
            mq_proto_library
            re2::re2
            rocketmq_interface
            uv_a
            zlib)
if (ENABLE_TRACING)
    target_link_libraries(rocketmq_static
        PUBLIC
            opentelemetry_trace
            opentelemetry_exporter_otprotocol)
endif ()

add_library(rocketmq_shared
        SHARED
            $<TARGET_OBJECTS:base_obj>
            $<TARGET_OBJECTS:admin_obj>
            $<TARGET_OBJECTS:scheduler_obj>
            $<TARGET_OBJECTS:client_obj>
            $<TARGET_OBJECTS:logger_obj>
            $<TARGET_OBJECTS:countdown_latch_obj>
            $<TARGET_OBJECTS:rocketmq_obj>)
target_link_libraries(rocketmq_shared
        PUBLIC
            absl::flat_hash_set
            absl::flat_hash_map
            absl::random_random
            absl::synchronization
            absl::strings
            fmt
            mq_proto_library
            re2::re2
            rocketmq_interface
            uv_a
            zlib)
if (ENABLE_TRACING)
    target_link_libraries(rocketmq_shared
        PUBLIC
            opentelemetry_trace
            opentelemetry_exporter_otprotocol)
endif ()

set_target_properties(rocketmq_static rocketmq_shared
        PROPERTIES
            ARCHIVE_OUTPUT_NAME rocketmq
            LIBRARY_OUTPUT_NAME rocketmq)

add_library(rocketmq ALIAS rocketmq_static)