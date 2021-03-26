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

set(CMAKE_PREFIX_PATH
    "${CMAKE_PREFIX_PATH};${CMAKE_SOURCE_DIR}/bin/lib64/cmake;${CMAKE_SOURCE_DIR}/bin/lib/cmake"
)

# Find dependencies find_package(GTest REQUIRED CONFIG)
if(NOT GTest_FOUND)
  include_directories("${CMAKE_SOURCE_DIR}/bin/include")

  if(EXISTS "${CMAKE_SOURCE_DIR}/bin/lib64/libgtest.a")
    set(Gtest_LIBRARY_DIR "${CMAKE_SOURCE_DIR}/bin/lib64")
  else()
    set(Gtest_LIBRARY_DIR "${CMAKE_SOURCE_DIR}/bin/lib")
  endif()
  link_libraries(
    "${Gtest_LIBRARY_DIR}/libgtest.a" "${Gtest_LIBRARY_DIR}/libgtest_main.a"
    "${Gtest_LIBRARY_DIR}/libgmock.a" "${Gtest_LIBRARY_DIR}/libgmock_main.a")
endif()

if(NOT (CMAKE_VERSION VERSION_LESS "3.9"))
  cmake_policy(SET CMP0054 NEW)
  cmake_policy(SET CMP0057 NEW)
  include(GoogleTest)
endif()

function(config_test file)
  get_filename_component(basename ${file} NAME_WE)

  add_executable(${basename} ${file})

  if(MSVC)
    if(CMAKE_CONFIGURATION_TYPES STREQUAL "Release")
      set_target_properties(${basename} PROPERTIES LINK_FLAGS
                                                   "/NODEFAULTLIB:LIBCMT")
    else()
      set_target_properties(${basename} PROPERTIES LINK_FLAGS
                                                   "/NODEFAULTLIB:LIBCMTD")
    endif()
  endif()

  if(GTest_FOUND)
    if(BUILD_ROCKETMQ_SHARED)
      target_link_libraries(${basename} rocketmq_shared GTest::gtest
                            GTest::gtest_main GTest::gmock GTest::gmock_main)
    else(BUILD_ROCKETMQ_SHARED)
      target_link_libraries(${basename} rocketmq_static GTest::gtest
                            GTest::gtest_main GTest::gmock GTest::gmock_main)
    endif(BUILD_ROCKETMQ_SHARED)
  else()
    if(BUILD_ROCKETMQ_SHARED)
      target_link_libraries(${basename} rocketmq_shared)
    else(BUILD_ROCKETMQ_SHARED)
      target_link_libraries(${basename} rocketmq_static)
    endif(BUILD_ROCKETMQ_SHARED)
  endif()

  if(NOT (CMAKE_VERSION VERSION_LESS "3.10"))
    gtest_discover_tests(${basename})
  elseif(NOT (CMAKE_VERSION VERSION_LESS "3.9"))
    gtest_add_tests(TARGET ${basename})
  endif()
endfunction()

function(config_all_test dir)
  file(GLOB files "${dir}/*")
  foreach(file ${files})
    if(IS_DIRECTORY ${file})
      config_all_test(${file})
    elseif(${file} MATCHES "^.+\\.(c|cpp)$")
      config_test(${file})
    endif()
  endforeach()
endfunction()

config_all_test(${PROJECT_SOURCE_DIR}/src)
