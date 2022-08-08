file(GLOB SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_library(ons OBJECT ${SRC_FILES})
target_include_directories(ons
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(ons
        PRIVATE
            api
            absl::strings
            asio
            base
            fmt
            proto
            client
            filesystem
            httplib
            log
            opencensus_api
            opencensus_proto
            rocketmq_stats
            rocketmq_trace
            otlp_exporter
            scheduler
            spdlog
            impl
        )