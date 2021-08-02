add_library(base_mocks INTERFACE)
target_include_directories(base_mocks INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(base_mocks
        INTERFACE
            base_library
            gmock)