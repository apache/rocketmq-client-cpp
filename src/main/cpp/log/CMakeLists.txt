add_library(logger_obj OBJECT)
target_sources(logger_obj PRIVATE Logger.cpp)
target_link_libraries(logger_obj
        PUBLIC
            admin_library
            ghc_filesystem)

add_library(logger_library
        STATIC
            $<TARGET_OBJECTS:logger_obj>)
target_link_libraries(logger_library
        PUBLIC
            admin_library
            ghc_filesystem)