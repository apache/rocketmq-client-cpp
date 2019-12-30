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

project(example)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)

include_directories(${CMAKE_SOURCE_DIR}/include)

link_directories(${LIBEVENT_LIBRARY})
link_directories(${JSONCPP_LIBRARY})

file(GLOB files RELATIVE "${PROJECT_SOURCE_DIR}" "*.c*")
list(REMOVE_ITEM files "ArgHelper.cpp")
foreach(file ${files})
    get_filename_component(basename ${file} NAME_WE)
    add_executable(${basename} ${file})
    if(MSVC)
        if(CMAKE_CONFIGURATION_TYPES STREQUAL "Release")
            set_target_properties( ${basename} PROPERTIES LINK_FLAGS "/NODEFAULTLIB:LIBCMT" )
        else()
            set_target_properties( ${basename} PROPERTIES LINK_FLAGS "/NODEFAULTLIB:LIBCMTD" )
        endif()
    endif()

    if (MSVC)
        if (BUILD_ROCKETMQ_SHARED)
            target_link_libraries (${basename}  rocketmq_shared ${deplibs} ${LIBEVENT_LIBRARIES} ${JSONCPP_LIBRARIES})
        else()
            target_link_libraries (${basename}  rocketmq_static ${deplibs} ${LIBEVENT_LIBRARIES} ${JSONCPP_LIBRARIES})
        endif()
    else()
        if (BUILD_ROCKETMQ_SHARED)
            target_link_libraries (${basename} rocketmq_shared)
        else()
            target_link_libraries (${basename} rocketmq_static)
        endif()
    endif()

endforeach()
