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


SET(SUB_DIRS)
file(GLOB children ${CMAKE_SOURCE_DIR}/src/*)
FOREACH(child ${children})
    IF(IS_DIRECTORY ${child})
        LIST(APPEND SUB_DIRS ${child})
    ENDIF()
ENDFOREACH()
LIST(APPEND SUB_DIRS ${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${SUB_DIRS})

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/test/bin)

include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${Boost_INCLUDE_DIRS})


set(Gtest_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/bin/include/gtest)
set(Gmock_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/bin/include/gmock)

include_directories(${Gtest_INCLUDE_DIR})
include_directories(${Gtest_INCLUDE_DIR}/internal)
include_directories(${Gmock_INCLUDE_DIR})
include_directories(${Gmock_INCLUDE_DIR}/internal)

set(Gtest_LIBRARY_DIRS ${CMAKE_SOURCE_DIR}/bin/lib)
set(Gtest_LIBRARIES ${Gtest_LIBRARY_DIRS}/libgtest_main.a;${Gtest_LIBRARY_DIRS}/libgtest.a)
set(Gmock_LIBRARIES ${Gtest_LIBRARY_DIRS}/libgmock_main.a;${Gtest_LIBRARY_DIRS}/libgmock.a)
message(status "** Gmock_LIBRARIES: ${Gmock_LIBRARIES}")

link_directories(${Boost_LIBRARY_DIRS})
link_directories(${LIBEVENT_LIBRARY})
link_directories(${JSONCPP_LIBRARY})

set(ROCKETMQ_LIBRARIES ${CMAKE_SOURCE_DIR}/bin/librocketmq.a)
message(status "ROCKETMQ_LIBRARIES ${ROCKETMQ_LIBRARIES}")

set(CMAKE_BUILD_TYPE "Debug")
file(GLOB files "src/*.c*")
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
        	target_link_libraries (${basename}  rocketmq_shared ${deplibs}
        	${Boost_LIBRARIES} ${LIBEVENT_LIBRARIES} ${JSONCPP_LIBRARIES} ${x`})
    	else()
        	target_link_libraries (${basename}  rocketmq_static ${deplibs}
        	${Boost_LIBRARIES} ${LIBEVENT_LIBRARIES} ${JSONCPP_LIBRARIES} ${Gtest_LIBRARIES})
        endif()
    else()
            target_link_libraries (${basename}  rocketmq_shared ${deplibs})
            target_link_libraries (${basename}  rocketmq_shared ${Gtest_LIBRARIES})
            target_link_libraries (${basename}  rocketmq_shared ${Gmock_LIBRARIES})
    endif()
    
endforeach()
