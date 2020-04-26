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

project(test)

set(CMAKE_BUILD_TYPE "Debug")

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH};${CMAKE_SOURCE_DIR}/bin/lib64/cmake;${CMAKE_SOURCE_DIR}/bin/lib/cmake")

include(GoogleTest)
find_package(GTest 1.10.0 REQUIRED CONFIG)

set(SUB_DIRS)
file(GLOB children ${CMAKE_SOURCE_DIR}/src/*)
foreach (child ${children})
    if(IS_DIRECTORY ${child})
        list(APPEND SUB_DIRS ${child})
    endif()
endforeach ()
list(APPEND SUB_DIRS ${CMAKE_SOURCE_DIR}/src)

include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${SUB_DIRS})

function(config_test file)
    get_filename_component(basename ${file} NAME_WE)

    add_executable(${basename} ${file})

    if(MSVC)
        if(CMAKE_CONFIGURATION_TYPES STREQUAL "Release")
            set_target_properties(${basename} PROPERTIES LINK_FLAGS "/NODEFAULTLIB:LIBCMT")
        else()
            set_target_properties(${basename} PROPERTIES LINK_FLAGS "/NODEFAULTLIB:LIBCMTD")
        endif()
    endif()

    if (BUILD_ROCKETMQ_SHARED)
        if (MSVC) 
            target_link_libraries(${basename} rocketmq_shared ${x`})
        else (MSVC)
            target_link_libraries(${basename} rocketmq_shared
                GTest::gtest GTest::gtest_main GTest::gmock GTest::gmock_main)
        endif (MSVC)
    else (BUILD_ROCKETMQ_SHARED)
        target_link_libraries(${basename} rocketmq_static
            GTest::gtest GTest::gtest_main GTest::gmock GTest::gmock_main)
    endif (BUILD_ROCKETMQ_SHARED)

    if (NOT (CMAKE_VERSION VERSION_LESS "3.10"))
        gtest_discover_tests(${basename})
    else ()
        gtest_add_tests(${basename} ${file})
    endif ()
endfunction()

function(config_all_test dir)
    file(GLOB files "${dir}/*")
    foreach (file ${files})
        if (IS_DIRECTORY ${file})
            config_all_test(${file})
        elseif (${file} MATCHES "^.+\\.(c|cpp)$")
            config_test(${file})
        endif ()
    endforeach ()
endfunction()

config_all_test(${PROJECT_SOURCE_DIR}/src)
