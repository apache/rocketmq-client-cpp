cmake_minimum_required(VERSION 3.13 FATAL_ERROR)
project(rocketmq VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_VERBOSE_MAKEFILE ON)
set(VERSION "1.0.0")

include(FetchContent)

find_package(Threads)

FetchContent_Declare(
        gRPC
        GIT_REPOSITORY https://gitee.com/lizhanhui/grpc.git
        GIT_TAG        v1.38.0-p2
)
FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.10.x
)
FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG        v1.8.1
)

FetchContent_Declare(
        uv
        GIT_REPOSITORY https://github.com/libuv/libuv.git
        GIT_TAG        v1.34.0
)

FetchContent_Declare(
        fmt
        GIT_REPOSITORY https://github.com/fmtlib/fmt.git
        GIT_TAG        7.0.2
)

FetchContent_Declare(
        ghc
        GIT_REPOSITORY https://github.com/gulrak/filesystem.git
        GIT_TAG        v1.3.2
)

FetchContent_Declare(
        opentelemetry
        GIT_REPOSITORY https://github.com/open-telemetry/opentelemetry-cpp.git
        GIT_TAG v1.0.0-rc4
)

set(FETCHCONTENT_QUIET OFF)
option(INSTALL_GTEST OFF)
set(BUILD_TESTING OFF)
set(ABSL_RUN_TESTS OFF)
set(SPDLOG_FMT_EXTERNAL ON)
if (BUILD_TESTS)
    FetchContent_MakeAvailable(googletest)
endif ()

FetchContent_MakeAvailable(fmt gRPC spdlog ghc opentelemetry)

option(ENABLE_TRACING OFF)
if (ENABLE_TRACING)
    FetchContent_MakeAvailable(opentelemetry)
endif ()

# Since FetchContent uses add_subdirectory under the hood, we can use
# the grpc targets directly from this build.
set(_PROTOBUF_LIBPROTOBUF libprotobuf)
set(_REFLECTION grpc++_reflection)
set(_PROTOBUF_PROTOC $<TARGET_FILE:protoc>)
set(_GRPC_GRPCPP grpc++)
if(CMAKE_CROSSCOMPILING)
    message("CMake cross compiling")
    find_program(_GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin)
else()
    set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:grpc_cpp_plugin>)
    message("CMake non-cross compiling: ${_GRPC_CPP_PLUGIN_EXECUTABLE}")
endif()

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/proto)
file(TOUCH ${CMAKE_CURRENT_BINARY_DIR}/proto/dummy.cc)
file(GLOB PROTO_FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/proto/apache/rocketmq/v1/*.proto"
        "${CMAKE_CURRENT_SOURCE_DIR}/proto/google/rpc/*.proto")
foreach(proto_file ${PROTO_FILES})
    get_filename_component(proto_file_base_name ${proto_file} NAME_WE)
    message("Proto file base file name: ${proto_file_base_name}, abs: ${proto_file}")
    set(proto_src_file "${CMAKE_BINARY_DIR}/proto/apache/rocketmq/v1/${proto_file_base_name}.pb.cc")
    set(proto_hdr_file "${CMAKE_BINARY_DIR}/proto/apache/rocketmq/v1/${proto_file_base_name}.pb.h")
    set(grpc_src_file  "${CMAKE_BINARY_DIR}/proto/apache/rocketmq/v1/${proto_file_base_name}.grpc.pb.cc")
    set(grpc_hdr_file  "${CMAKE_BINARY_DIR}/proto/apache/rocketmq/v1/${proto_file_base_name}.grpc.pb.h")

    add_custom_command(
        OUTPUT "${proto_src_file}" "${proto_hdr_file}" "${grpc_src_file}" "${grpc_hdr_file}"
        COMMAND ${_PROTOBUF_PROTOC}
        ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}/proto"
        --cpp_out "${CMAKE_BINARY_DIR}/proto"
        -I "${CMAKE_SOURCE_DIR}/proto"
        --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
        "${proto_file}"
        DEPENDS "${proto_file}"
        COMMENT "Protoc to generate gRPC stub files"
    )

    add_custom_target(${proto_file_base_name}
            ALL
            DEPENDS ${proto_src_file} ${proto_hdr_file} ${grpc_src_file} ${grpc_hdr_file}
            COMMENT "Custom target"
            VERBATIM)
endforeach()

file(GLOB_RECURSE STUB_FILES ${CMAKE_BINARY_DIR}/proto/*.cc)
add_library(mq_proto_library STATIC ${STUB_FILES})
target_include_directories(mq_proto_library PUBLIC ${CMAKE_BINARY_DIR}/proto)
target_link_libraries(mq_proto_library
        PUBLIC
            ${_REFLECTION}
            ${_GRPC_GRPCPP}
            ${_PROTOBUF_LIBPROTOBUF})

enable_testing()

option(ENABLE_TLS ON)
if (ENABLE_TLS)
    message("TLS enabled")
    find_package(OpenSSL REQUIRED)
    add_compile_definitions(TLS_ENABLED=ON)
endif ()

add_subdirectory(api)
add_subdirectory(src/main/cpp)

if (BUILD_TESTS)
    add_subdirectory(src/test/cpp/ut)
endif()

if (BUILD_INTEGRATION_TESTS)
    add_subdirectory(src/test/cpp/it)
endif ()

if (BUILD_EXAMPLES)
    add_subdirectory(example)
endif ()

include(GNUInstallDirs)
install(DIRECTORY api/rocketmq
            DESTINATION include)
install(TARGETS rocketmq_static rocketmq_shared
            LIBRARY DESTINATION lib)
install(FILES
            example/rocketmq/ExampleProducer.cpp
            example/rocketmq/ExamplePushConsumer.cpp
        DESTINATION example)

set(CPACK_PACKAGE_VERSION ${VERSION})
set(CPACK_GENERATOR "RPM")
set(CPACK_PACKAGE_NAME "rocketmq-cpp")
set(CPACK_PACKAGE_RELEASE 1)
set(CPACK_PACKAGE_CONTACT "Zhanhui Li")
set(CPACK_PACKAGE_VENDOR "Alibaba")
set(CPACK_PACKAGING_INSTALL_PREFIX "/home/admin")
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CPACK_PACKAGE_RELEASE}")
include(CPack)