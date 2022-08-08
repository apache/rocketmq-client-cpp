add_library(otlp_exporter OBJECT OtlpExporter.cpp)
target_include_directories(otlp_exporter
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(otlp_exporter
        PRIVATE
            absl::flat_hash_map
            api
            base
            client
            fmt
            httplib
            opencensus::stats
            proto
            scheduler
            spdlog)