add_library(countdown_latch_obj OBJECT)
target_sources(countdown_latch_obj PRIVATE CountdownLatch.cpp)
target_include_directories(countdown_latch_obj
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(countdown_latch_obj
        PUBLIC absl::strings
        PUBLIC absl::base
        PUBLIC absl::synchronization
        PUBLIC base_library
        PRIVATE spdlog::spdlog_header_only)

add_library(countdown_latch_library STATIC $<TARGET_OBJECTS:countdown_latch_obj>)