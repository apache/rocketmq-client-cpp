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

project(rocketmq-client)

option(BUILD_ROCKETMQ_STATIC "build rocketmq-client static library" OFF)
option(BUILD_ROCKETMQ_SHARED "build rocketmq-client shared library" ON)

# source files
file(GLOB_RECURSE SRC_FILES ${PROJECT_SOURCE_DIR}/../src/*.c*)
list(REMOVE_ITEM SRC_FILES ${PROJECT_SOURCE_DIR}/../src/dllmain.cpp)

# subdirs
set(SUB_DIRS)
file(GLOB children ${PROJECT_SOURCE_DIR}/../src/*)
foreach (child ${children})
    if (IS_DIRECTORY ${child})
        list(APPEND SUB_DIRS ${child})
    endif ()
endforeach ()
list(APPEND SUB_DIRS ${PROJECT_SOURCE_DIR}/../src)

# libs_directories
file(GLOB LIB_DIRS ${PROJECT_SOURCE_DIR}/../libs/*)
foreach (dir ${LIB_DIRS})
    if (IS_DIRECTORY ${dir})
        set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH};${dir})
        include_directories(${dir}/include)
    endif ()
endforeach ()

# static
if (BUILD_ROCKETMQ_STATIC)
    add_library(rocketmq_static STATIC
            ${SRC_FILES})
    target_include_directories(rocketmq_static
            PUBLIC ${PROJECT_SOURCE_DIR}/../include ${SUB_DIRS})
    if (spdlog_FOUND)
        target_link_libraries(rocketmq_static
                PUBLIC ${deplibs} Signature ${JSONCPP_LIBRARIES} ${LIBEVENT_LIBRARIES} spdlog::spdlog)
    else (spdlog_FOUND)
        target_link_libraries(rocketmq_static
                PUBLIC ${deplibs} Signature ${JSONCPP_LIBRARIES} ${LIBEVENT_LIBRARIES})
    endif (spdlog_FOUND)
    # set_target_properties(rocketmq_static
    #         PROPERTIES OUTPUT_NAME "rocketmq")

    include(BundleStaticLibrary)
    bundle_static_library(rocketmq_static rocketmq)
endif ()

# shared
if (BUILD_ROCKETMQ_SHARED)
    add_library(rocketmq_shared SHARED
            ${SRC_FILES})
    target_include_directories(rocketmq_shared
            PUBLIC ${PROJECT_SOURCE_DIR}/../include ${SUB_DIRS})
    if (spdlog_FOUND)
        target_link_libraries(rocketmq_shared
                PUBLIC ${deplibs} Signature ${JSONCPP_LIBRARIES} ${LIBEVENT_LIBRARIES} spdlog::spdlog)
    else (spdlog_FOUND)
        target_link_libraries(rocketmq_shared
                PUBLIC ${deplibs} Signature ${JSONCPP_LIBRARIES} ${LIBEVENT_LIBRARIES})
    endif (spdlog_FOUND)
    set_target_properties(rocketmq_shared
            PROPERTIES OUTPUT_NAME "rocketmq")
endif ()

# install
if (BUILD_ROCKETMQ_STATIC)
    install(TARGETS rocketmq_static DESTINATION lib)
    install(FILES ${LIBRARY_OUTPUT_PATH}/${CMAKE_STATIC_LIBRARY_PREFIX}rocketmq${CMAKE_STATIC_LIBRARY_SUFFIX} DESTINATION lib)
endif ()
if (BUILD_ROCKETMQ_SHARED)
    install(TARGETS rocketmq_shared DESTINATION lib)
endif ()
install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/ DESTINATION include/rocketmq)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/doc/ DESTINATION doc)
