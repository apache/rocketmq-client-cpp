add_library(logger_obj OBJECT)
target_sources(logger_obj
        PRIVATE
            LoggerImpl.cpp)
target_link_libraries(logger_obj
        PUBLIC
            rocketmq_interface
            ghc_filesystem
            spdlog::spdlog_header_only)
target_include_directories(logger_obj
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(logger_library
        STATIC
            $<TARGET_OBJECTS:logger_obj>)
target_include_directories(logger_library
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(logger_library
        PUBLIC
            rocketmq_interface
            ghc_filesystem
            spdlog::spdlog_header_only)