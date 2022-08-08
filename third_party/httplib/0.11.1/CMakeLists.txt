add_library(httplib INTERFACE)
target_include_directories(httplib
        INTERFACE
            ${CMAKE_CURRENT_SOURCE_DIR}/include)