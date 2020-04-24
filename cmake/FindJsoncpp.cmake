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

# Find jsoncpp
#
# Find the jsoncpp includes and library
#
# if you nee to add a custom library search path, do it via CMAKE_PREFIX_PATH
#
# -*- cmake -*-
# - Find JSONCpp
# Find the JSONCpp includes and library
#
# This module define the following variables:
#
#  JSONCPP_FOUND, If false, do not try to use jsoncpp.
#  JSONCPP_INCLUDE_DIRS, where to find json.h, etc.
#  JSONCPP_LIBRARIES, the libraries needed to use jsoncpp.

# Support preference of static libs by adjusting CMAKE_FIND_LIBRARY_SUFFIXES
if (JSONCPP_USE_STATIC_LIBS)
    set(_jsoncpp_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES :${CMAKE_FIND_LIBRARY_SUFFIXES})
    if (WIN32)
        list(INSERT CMAKE_FIND_LIBRARY_SUFFIXES 0 .lib .a)
    else ()
        set(CMAKE_FIND_LIBRARY_SUFFIXES .a)
    endif ()
else ()
    set(_jsoncpp_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES :${CMAKE_FIND_LIBRARY_SUFFIXES})
    if (WIN32)
        list(INSERT CMAKE_FIND_LIBRARY_SUFFIXES 0 .dll .so)
    elseif (APPLE)
        set(CMAKE_FIND_LIBRARY_SUFFIXES .dylib)
    else ()
        set(CMAKE_FIND_LIBRARY_SUFFIXES .so)
    endif ()
endif ()

set(JSONCPP_INCLUDE_SEARCH_PATH ${CMAKE_SOURCE_DIR}/bin/include /usr/local/include /usr/include)
set(JSONCPP_LIBRARIES_SEARCH_PATH ${CMAKE_SOURCE_DIR}/bin/lib /usr/local/lib /usr/lib)
if (JSONCPP_ROOT)
    list(INSERT JSONCPP_INCLUDE_SEARCH_PATH 0 ${JSONCPP_ROOT}/include)
    list(INSERT JSONCPP_LIBRARIES_SEARCH_PATH 0 ${JSONCPP_ROOT}/lib)
endif ()

find_path(JSONCPP_INCLUDE_DIRS
        NAMES json.h json/json.h
        PATHS ${JSONCPP_INCLUDE_SEARCH_PATH}
        PATH_SUFFIXES jsoncpp)

find_library(JSONCPP_LIBRARIES
        NAMES jsoncpp
        PATHS ${JSONCPP_LIBRARIES_SEARCH_PATH})

if (JSONCPP_LIBRARIES AND JSONCPP_INCLUDE_DIRS)
    set(JSONCPP_FOUND "YES")
else (JSONCPP_LIBRARIES AND JSONCPP_INCLUDE_DIRS)
    set(JSONCPP_FOUND "NO")
endif (JSONCPP_LIBRARIES AND JSONCPP_INCLUDE_DIRS)

if (JSONCPP_FOUND)
    if (NOT JSONCPP_FIND_QUIETLY)
        message(STATUS "Found JSONCpp: ${JSONCPP_LIBRARIES}")
    endif (NOT JSONCPP_FIND_QUIETLY)
else (JSONCPP_FOUND)
    if (JSONCPP_FIND_REQUIRED)
        message(FATAL_ERROR "Could not find JSONCPP library, include: ${JSONCPP_INCLUDE_DIRS}, lib: ${JSONCPP_LIBRARIES}")
    endif (JSONCPP_FIND_REQUIRED)
endif (JSONCPP_FOUND)

mark_as_advanced(JSONCPP_LIBRARIES JSONCPP_INCLUDE_DIRS)

# Restore the original find library ordering
if (JSONCPP_USE_STATIC_LIBS)
    set(CMAKE_FIND_LIBRARY_SUFFIXES ${_jsoncpp_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES})
endif ()
