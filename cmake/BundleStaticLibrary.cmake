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
# ref: https://cristianadam.eu/20190501/bundling-together-static-libraries-with-cmake/

set(STATIC_LIBRARY_REGEX "${CMAKE_STATIC_LIBRARY_SUFFIX}")
string(REPLACE "." "\\." STATIC_LIBRARY_REGEX "${STATIC_LIBRARY_REGEX}")
set(STATIC_LIBRARY_REGEX "^.+${STATIC_LIBRARY_REGEX}$")

function(bundle_static_library tgt_name bundled_tgt_name)
  list(APPEND static_libs ${tgt_name})
  set(static_libraries)

  function(_recursively_collect_dependencies input_target)
    set(_input_link_libraries LINK_LIBRARIES)
    get_target_property(_input_type ${input_target} TYPE)
    if(${_input_type} STREQUAL "INTERFACE_LIBRARY")
      set(_input_link_libraries INTERFACE_LINK_LIBRARIES)
    endif()
    get_target_property(public_dependencies ${input_target}
                        ${_input_link_libraries})
    foreach(dependency IN LISTS public_dependencies)
      if(TARGET ${dependency})
        get_target_property(alias ${dependency} ALIASED_TARGET)
        if(TARGET ${alias})
          set(dependency ${alias})
        endif()
        get_target_property(_type ${dependency} TYPE)
        if(${_type} STREQUAL "STATIC_LIBRARY")
          list(APPEND static_libs ${dependency})
        endif()

        get_property(library_already_added GLOBAL
                     PROPERTY _${tgt_name}_static_bundle_${dependency})
        if(NOT library_already_added)
          set_property(GLOBAL PROPERTY _${tgt_name}_static_bundle_${dependency}
                                       ON)
          _recursively_collect_dependencies(${dependency})
        endif()
      else()
        string(REGEX MATCH ${STATIC_LIBRARY_REGEX} IS_STATIC_LIBRARY
                     ${dependency})
        if(IS_STATIC_LIBRARY)
          list(APPEND static_libs ${dependency})
        endif()
      endif()
    endforeach()
    set(static_libs
        ${static_libs}
        PARENT_SCOPE)
  endfunction()

  _recursively_collect_dependencies(${tgt_name})

  list(REMOVE_DUPLICATES static_libs)

  set(bundled_tgt_full_name
      ${LIBRARY_OUTPUT_PATH}/${CMAKE_STATIC_LIBRARY_PREFIX}${bundled_tgt_name}${CMAKE_STATIC_LIBRARY_SUFFIX}
  )

  message(STATUS "Bundling static library: ${bundled_tgt_full_name}")
  if(CMAKE_CXX_COMPILER_ID MATCHES "^(AppleClang|Clang|GNU)$")
    if(APPLE)
      find_program(LIB_TOOL libtool REQUIRED)

      foreach(tgt IN LISTS static_libs)
        if(TARGET ${tgt})
          list(APPEND static_libs_full_names $<TARGET_FILE:${tgt}>)
        else()
          list(APPEND static_libs_full_names ${tgt})
        endif()
      endforeach()

      add_custom_command(
        OUTPUT ${bundled_tgt_full_name}
        COMMAND ${LIB_TOOL} -no_warning_for_no_symbols -static -o
                ${bundled_tgt_full_name} ${static_libs_full_names}
        COMMENT "Bundling ${bundled_tgt_name}"
        VERBATIM)
    else()
      file(WRITE ${CMAKE_BINARY_DIR}/${bundled_tgt_name}.mri.in
           "CREATE ${bundled_tgt_full_name}\n")

      foreach(tgt IN LISTS static_libs)
        if(TARGET ${tgt})
          file(APPEND ${CMAKE_BINARY_DIR}/${bundled_tgt_name}.mri.in
               "ADDLIB $<TARGET_FILE:${tgt}>\n")
        else()
          file(APPEND ${CMAKE_BINARY_DIR}/${bundled_tgt_name}.mri.in
               "ADDLIB ${tgt}\n")
        endif()
      endforeach()

      file(APPEND ${CMAKE_BINARY_DIR}/${bundled_tgt_name}.mri.in "SAVE\n")
      file(APPEND ${CMAKE_BINARY_DIR}/${bundled_tgt_name}.mri.in "END\n")

      file(
        GENERATE
        OUTPUT ${CMAKE_BINARY_DIR}/${bundled_tgt_name}.mri
        INPUT ${CMAKE_BINARY_DIR}/${bundled_tgt_name}.mri.in)

      set(AR_TOOL ${CMAKE_AR})
      if(CMAKE_INTERPROCEDURAL_OPTIMIZATION)
        set(AR_TOOL ${CMAKE_CXX_COMPILER_AR})
      endif()

      add_custom_command(
        OUTPUT ${bundled_tgt_full_name}
        COMMAND ${AR_TOOL} -M < ${CMAKE_BINARY_DIR}/${bundled_tgt_name}.mri
        COMMENT "Bundling ${bundled_tgt_name}"
        VERBATIM)
    endif()
  elseif(MSVC)
    find_program(LIB_TOOL lib REQUIRED)

    foreach(tgt IN LISTS static_libs)
      list(APPEND static_libs_full_names $<TARGET_FILE:${tgt}>)
    endforeach()

    add_custom_command(
      OUTPUT ${bundled_tgt_full_name}
      COMMAND ${LIB_TOOL} /NOLOGO /OUT:${bundled_tgt_full_name}
              ${static_libs_full_names}
      COMMENT "Bundling ${bundled_tgt_name}"
      VERBATIM)
  else()
    message(FATAL_ERROR "Unknown bundle scenario!")
  endif()

  add_custom_target("${bundled_tgt_name}_" ALL DEPENDS ${bundled_tgt_full_name})
  add_dependencies("${bundled_tgt_name}_" ${tgt_name})

  add_library(${bundled_tgt_name} STATIC IMPORTED)
  set_target_properties(
    ${bundled_tgt_name}
    PROPERTIES IMPORTED_LOCATION ${bundled_tgt_full_name}
               INTERFACE_INCLUDE_DIRECTORIES
               $<TARGET_PROPERTY:${tgt_name},INTERFACE_INCLUDE_DIRECTORIES>)
  add_dependencies(${bundled_tgt_name} "${bundled_tgt_name}_")

endfunction()
