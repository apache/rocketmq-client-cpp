add_executable(scheduler_test SchedulerTest.cpp)
target_link_libraries(scheduler_test
        PRIVATE
            absl::synchronization
            base_library
            gtest_main
            scheduler_library)
add_test(ut_scheduler_test scheduler_test)