function(add_example name file)
    add_executable(${name} ${file})
    target_link_libraries(${name} PRIVATE api ZLIB::ZLIB rocketmq)
endfunction()

add_example(ons_example_producer ons/ExampleProducer.cpp)
add_example(ons_example_push_consumer ons/ExamplePushConsumer.cpp)