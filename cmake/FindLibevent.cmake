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
#
# - Try to find libevent
#.rst
# FindLibevent
# ------------
#
# Find Libevent include directories and libraries. Invoke as::
#
#   find_package(Libevent
#     [version] [EXACT]   # Minimum or exact version
#     [REQUIRED]          # Fail if Libevent is not found
#     [COMPONENT <C>...]) # Libraries to look for
#
# Valid components are one or more of:: libevent core extra pthreads openssl.
# Note that 'libevent' contains both core and extra. You must specify one of
# them for the other components.
#
# This module will define the following variables:
#
#  LIBEVENT_FOUND        - True if headers and requested libraries were found
#  LIBEVENT_INCLUDE_DIRS - Libevent include directories
#  LIBEVENT_LIBRARIES    - Libevent libraries to be linked
#  LIBEVENT_<C>_FOUND    - Component <C> was found (<C> is uppercase)
#  LIBEVENT_<C>_LIBRARY  - Library to be linked for Libevent component <C>.

# Support preference of static libs by adjusting CMAKE_FIND_LIBRARY_SUFFIXES
if(Libevent_USE_STATIC_LIBS)
  set(_libevent_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES
      :${CMAKE_FIND_LIBRARY_SUFFIXES})
  if(WIN32)
    list(INSERT CMAKE_FIND_LIBRARY_SUFFIXES 0 .lib .a)
  else()
    set(CMAKE_FIND_LIBRARY_SUFFIXES .a)
  endif()
else()
  set(_libevent_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES
      :${CMAKE_FIND_LIBRARY_SUFFIXES})
  if(WIN32)
    list(INSERT CMAKE_FIND_LIBRARY_SUFFIXES 0 .dll .so)
  elseif(APPLE)
    set(CMAKE_FIND_LIBRARY_SUFFIXES .dylib)
  else()
    set(CMAKE_FIND_LIBRARY_SUFFIXES .so)
  endif()
endif()

# default search path
set(LIBEVENT_INCLUDE_SEARCH_PATH /usr/local/include /usr/include)
set(LIBEVENT_LIBRARIES_SEARCH_PATH /usr/local/lib /usr/lib)

# pkgconfig hint
find_package(PkgConfig QUIET)
pkg_check_modules(PC_LIBEVENT QUIET libevent)
if(PC_LIBEVENT_FOUND)
  list(INSERT LIBEVENT_INCLUDE_SEARCH_PATH 0 ${PC_LIBEVENT_INCLUDE_DIRS})
  list(INSERT LIBEVENT_LIBRARIES_SEARCH_PATH 0 ${PC_LIBEVENT_LIBRARY_DIRS})
endif()

# custom search path
if(LIBEVENT_ROOT)
  list(INSERT LIBEVENT_INCLUDE_SEARCH_PATH 0 ${LIBEVENT_ROOT}/include)
  list(INSERT LIBEVENT_LIBRARIES_SEARCH_PATH 0 ${LIBEVENT_ROOT}/lib)
endif()

set(_LIBEVENT_REQUIRED_VARS LIBEVENT_EVENT_CONFIG_DIR)

# Look for the Libevent 2.0 or 1.4 headers
find_path(
  LIBEVENT_EVENT_CONFIG_DIR
  NAMES event2/event-config.h event-config.h
  PATHS ${LIBEVENT_INCLUDE_SEARCH_PATH}
  NO_DEFAULT_PATH)

# Parse version
if(LIBEVENT_EVENT_CONFIG_DIR)
  set(_version_regex "^#define[ \t]+_EVENT_VERSION[ \t]+\"([^\"]+)\".*")
  if(EXISTS "${LIBEVENT_EVENT_CONFIG_DIR}/event2/event-config.h")
    # Libevent 2.0
    file(STRINGS "${LIBEVENT_EVENT_CONFIG_DIR}/event2/event-config.h"
         LIBEVENT_VERSION REGEX "${_version_regex}")
    if(NOT LIBEVENT_VERSION)
      # Libevent 2.1
      set(_version_regex "^#define[ \t]+EVENT__VERSION[ \t]+\"([^\"]+)\".*")
      file(STRINGS "${LIBEVENT_EVENT_CONFIG_DIR}/event2/event-config.h"
           LIBEVENT_VERSION REGEX "${_version_regex}")
    endif()
  else()
    # Libevent 1.4
    if(EXISTS "${LIBEVENT_EVENT_CONFIG_DIR}/event-config.h")
      file(STRINGS "${LIBEVENT_EVENT_CONFIG_DIR}/event-config.h"
           LIBEVENT_VERSION REGEX "${_version_regex}")
    endif()
  endif()
  string(REGEX REPLACE "${_version_regex}" "\\1" LIBEVENT_VERSION
                       "${LIBEVENT_VERSION}")
  unset(_version_regex)
endif()

# Prefix initialization
if(WIN32)
  set(Libevent_LIB_PREFIX "lib")
else()
  set(Libevent_LIB_PREFIX "")
endif()

if(WIN32)
  set(Libevent_FIND_COMPONENTS ${Libevent_LIB_PREFIX}event core extra)
else()
  set(Libevent_FIND_COMPONENTS ${Libevent_LIB_PREFIX}event core extra pthreads)
endif()

message(STATUS "** libevent components: ${Libevent_FIND_COMPONENTS}")
foreach(COMPONENT ${Libevent_FIND_COMPONENTS})
  set(_LIBEVENT_LIBNAME "${Libevent_LIB_PREFIX}event")
  # Note: compare two variables to avoid a CMP0054 policy warning
  if(NOT (COMPONENT STREQUAL _LIBEVENT_LIBNAME))
    set(_LIBEVENT_LIBNAME "${Libevent_LIB_PREFIX}event_${COMPONENT}")
  endif()
  string(TOUPPER "${COMPONENT}" COMPONENT_UPPER)
  message(
    STATUS "** find ${_LIBEVENT_LIBNAME} in ${LIBEVENT_LIBRARIES_SEARCH_PATH}")
  find_library(
    LIBEVENT_${COMPONENT_UPPER}_LIBRARY
    NAMES ${_LIBEVENT_LIBNAME}
    PATHS ${LIBEVENT_LIBRARIES_SEARCH_PATH}
    NO_DEFAULT_PATH)
  if(LIBEVENT_${COMPONENT_UPPER}_LIBRARY)
    set(LIBEVENT_${COMPONENT_UPPER}_FOUND ON)
  else()
    set(LIBEVENT_${COMPONENT_UPPER}_FOUND OFF)
  endif()
  list(APPEND _LIBEVENT_REQUIRED_VARS LIBEVENT_${COMPONENT_UPPER}_LIBRARY)
endforeach()
unset(_LIBEVENT_LIBNAME)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  Libevent
  REQUIRED_VARS ${_LIBEVENT_REQUIRED_VARS}
  VERSION_VAR LIBEVENT_VERSION
  HANDLE_COMPONENTS)
unset(_LIBEVENT_REQUIRED_VARS)

if(LIBEVENT_FOUND)
  set(LIBEVENT_INCLUDE_DIRS ${LIBEVENT_EVENT_CONFIG_DIR})
  set(LIBEVENT_LIBRARIES)
  foreach(COMPONENT ${Libevent_FIND_COMPONENTS})
    string(TOUPPER "${COMPONENT}" COMPONENT_UPPER)
    if(LIBEVENT_${COMPONENT_UPPER}_FOUND)
      list(APPEND LIBEVENT_LIBRARIES ${LIBEVENT_${COMPONENT_UPPER}_LIBRARY})
    endif()
  endforeach()
endif()
unset(LIBEVENT_EVENT_CONFIG_DIR)

mark_as_advanced(LIBEVENT_INCLUDE_DIRS LIBEVENT_LIBRARIES)

# Restore the original find library ordering
if(Libevent_USE_STATIC_LIBS)
  set(CMAKE_FIND_LIBRARY_SUFFIXES ${_libevent_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES})
endif()
