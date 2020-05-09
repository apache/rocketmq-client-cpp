# Licensed to the Apache Software Foundation (ASF) under one or more
# contributor license agreements.  See the NOTICE file distributed with
# this work for additional information regarding copyright ownership.
# The ASF licenses this file to You under the Apache License, Version 2.0
# (the "License"); you may not use this file except in compliance with
# the License.  You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 2.8)

# CMake complains if we don't have this.
if (COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)
endif ()

# We're escaping quotes in the Windows version number, because
# for some reason CMake won't do it at config version 2.4.7
# It seems that this restores the newer behaviour where define
# args are not auto-escaped.
if (COMMAND cmake_policy)
    cmake_policy(SET CMP0005 NEW)
endif ()

# First, declare project (important for prerequisite checks).
project(rocketmq-client-cpp)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif ()
if (NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_CONFIGURATION_TYPES "Release")
endif ()

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

set(CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS ON)
set(CMAKE_VERBOSE_MAKEFILE 1)

if (APPLE)
    set(CMAKE_MACOSX_RPATH 1)
endif (APPLE)

# put binaries in a different dir to make them easier to find.
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

# Find dependencies
#find_package(spdlog REQUIRED)
if (NOT spdlog_FOUND)
    include_directories(${CMAKE_SOURCE_DIR}/bin/include)
endif ()

option(Libevent_USE_STATIC_LIBS "only find libevent static libs" OFF) # only find static libs
if (NOT LIBEVENT_ROOT)
    set(LIBEVENT_ROOT ${CMAKE_SOURCE_DIR}/bin)
endif ()
find_package(Libevent 2.0.21 REQUIRED)
message(STATUS "** LIBEVENT_INCLUDE_DIR: ${LIBEVENT_INCLUDE_DIR}")
message(STATUS "** LIBEVENT_LIBRARIES: ${LIBEVENT_LIBRARIES}")

option(JSONCPP_USE_STATIC_LIBS "only find jsoncpp static libs" OFF) # only find static libs
if (NOT JSONCPP_ROOT)
    set(JSONCPP_ROOT ${CMAKE_SOURCE_DIR}/bin)
endif ()
find_package(Jsoncpp 0.10.6 REQUIRED)
message(STATUS "** JSONCPP_INCLUDE_DIRS: ${JSONCPP_INCLUDE_DIRS}")
message(STATUS "** JSONCPP_LIBRARIES: ${JSONCPP_LIBRARIES}")

# Set compile options
include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)
CHECK_C_COMPILER_FLAG("-std=c99" COMPILER_SUPPORTS_C99)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
if (COMPILER_SUPPORTS_C99 AND COMPILER_SUPPORTS_CXX11)
    if (NOT (CMAKE_VERSION VERSION_LESS "3.1"))
        set(CMAKE_C_STANDARD 99)
        set(CMAKE_CXX_STANDARD 11)
        message(STATUS "** set CMAKE_C_STANDARD to 99")
        message(STATUS "** set CMAKE_CXX_STANDARD to 11")
    else ()
        if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
            set(C_STANDARD_FLAG "-std=gnu99")
            set(CXX_STANDARD_FLAG "-std=gnu++11")
        else ()
            set(C_STANDARD_FLAG "-std=c99")
            set(CXX_STANDARD_FLAG "-std=c++11")
        endif ()
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${C_STANDARD_FLAG}")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CXX_STANDARD_FLAG}")
        message(STATUS "** set CMAKE_C_FLAGS with ${C_STANDARD_FLAG}")
        message(STATUS "** set CMAKE_CXX_FLAGS with ${CXX_STANDARD_FLAG}")
    endif ()
else ()
    message(FATAL_ERROR "The compiler has no C99 or C++11 support.")
endif ()

if (WIN32)
    add_definitions(-DWIN32 -DROCKETMQCLIENT_EXPORTS)
    add_compile_options(/EHsc)
    if (CMAKE_BUILD_TYPE EQUAL "Release")
        add_compile_options(/MT)
    else ()
        add_compile_options(/MTd)
    endif ()
else ()
    add_compile_options(-Wall
        -Wno-deprecated
        -fPIC
        -fno-strict-aliasing
        -Wno-unused-local-typedef
        -Wno-expansion-to-defined)

    if (CMAKE_BUILD_BITS EQUAL 32)
        add_compile_options(-m32)
    else () #not-condition
        add_compile_options(-m64)
    endif ()

    # Declare deplibs, so we can use list in linker later. There's probably
    # a more elegant way of doing this; with SCons, when you check for the
    # lib, it is automatically passed to the linker.
    set(deplibs)

    # For some reason, the check_function_exists macro doesn't detect
    # the inet_aton on some pure Unix platforms (e.g. sunos5). So we
    # need to do a more detailed check and also include some extra deplibs.
    list(APPEND deplibs dl)
    list(APPEND deplibs pthread)
    if (NOT APPLE)
        list(APPEND deplibs rt)
    endif ()
    list(APPEND deplibs z)

    option(CODE_COVERAGE "Enable coverage reporting" OFF)
    if (CODE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        # Add required flags (GCC & LLVM/Clang)
        # Code Coverage Configuration
        add_library(coverage_config INTERFACE)
        target_compile_options(coverage_config INTERFACE
                -O0        # no optimization
                -g         # generate debug info
                --coverage # sets all required flags
                )
        if (NOT (CMAKE_VERSION VERSION_LESS "3.13"))
            target_link_options(coverage_config INTERFACE --coverage)
        else ()
            target_link_libraries(coverage_config INTERFACE --coverage)
        endif ()
        list(APPEND deplibs coverage_config)
    endif (CODE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")

    # add include dir for bsd (posix uses /usr/include/)
    set(CMAKE_INCLUDE_PATH "${CMAKE_INCLUDE_PATH}:/usr/local/include")
endif ()

add_subdirectory(libs)
add_subdirectory(project)
add_subdirectory(example)

option(RUN_UNIT_TEST "RUN_UNIT_TEST" OFF)
if (RUN_UNIT_TEST)
    message(STATUS "** RUN_UNIT_TEST: Do execution testing")
    enable_testing()
    add_subdirectory(test)
endif ()
