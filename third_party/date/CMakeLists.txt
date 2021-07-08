add_library(date_library INTERFACE)
target_include_directories(date_library
        INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
add_library(date::date ALIAS date_library)