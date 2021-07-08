add_library(scheduler_obj OBJECT)
target_sources(scheduler_obj
        PRIVATE
            Scheduler.cpp)
target_include_directories(scheduler_obj
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(scheduler_obj
        PUBLIC
            base_library
            countdown_latch_library
            thread_pool)

add_library(scheduler_library
        STATIC
            $<TARGET_OBJECTS:scheduler_obj>)
target_include_directories(scheduler_library
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(scheduler_library
        PUBLIC
            base_library
            countdown_latch_library
            thread_pool)