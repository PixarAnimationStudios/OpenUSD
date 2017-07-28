#
# Copyright 2017 Pixar
#
# Licensed under the Apache License, Version 2.0 (the "Apache License")
# with the following modification; you may not use this file except in
# compliance with the Apache License and the following modification to it:
# Section 6. Trademarks. is deleted and replaced with:
#
# 6. Trademarks. This License does not grant permission to use the trade
#    names, trademarks, service marks, or product names of the Licensor
#    and its affiliates, except as required to comply with Section 4(c) of
#    the License and to reproduce the content of the NOTICE file.
#
# You may obtain a copy of the Apache License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the Apache License with the above modification is
# distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied. See the Apache License for the specific
# language governing permissions and limitations under the Apache License.
#
#=============================================================================
#
# The module defines the following variables:
#   EMBREE_INCLUDE_DIR - path to embree header directory
#   EMBREE_LIBRARY     - path to embree library file
#       EMBREE_FOUND   - true if embree was found
#
# Example usage:
#   find_package(EMBREE)
#   if(EMBREE_FOUND)
#     message("EMBREE found: ${EMBREE_LIBRARY}")
#   endif()
#
#=============================================================================

if (APPLE)
    set (EMBREE_LIB_NAME libembree.dylib)
elseif (UNIX)
    set (EMBREE_LIB_NAME libembree.so)
elseif (WIN32)
    set (EMBREE_LIB_NAME embree.lib)
endif()

find_library(EMBREE_LIBRARY
        "${EMBREE_LIB_NAME}"
    HINTS
        "${EMBREE_LOCATION}/lib64"
        "${EMBREE_LOCATION}/lib"
        "$ENV{EMBREE_LOCATION}/lib64"
        "$ENV{EMBREE_LOCATION}/lib"
    DOC
        "Embree library path"
)

find_path(EMBREE_INCLUDE_DIR
    embree2/rtcore.h
HINTS
    "${EMBREE_LOCATION}/include"
    "$ENV{EMBREE_LOCATION}/include"
DOC
    "Embree headers path"
)

if (EMBREE_INCLUDE_DIR AND EXISTS "${EMBREE_INCLUDE_DIR}/embree2/rtcore_version.h" )
    file(STRINGS "${EMBREE_INCLUDE_DIR}/embree2/rtcore_version.h" TMP REGEX "^#define RTCORE_VERSION_MAJOR.*$")
    string(REGEX MATCHALL "[0-9]+" MAJOR ${TMP})
    file(STRINGS "${EMBREE_INCLUDE_DIR}/embree2/rtcore_version.h" TMP REGEX "^#define RTCORE_VERSION_MINOR.*$")
    string(REGEX MATCHALL "[0-9]+" MINOR ${TMP})
    file(STRINGS "${EMBREE_INCLUDE_DIR}/embree2/rtcore_version.h" TMP REGEX "^#define RTCORE_VERSION_PATCH.*$")
    string(REGEX MATCHALL "[0-9]+" PATCH ${TMP})

    set (EMBREE_VERSION ${MAJOR}.${MINOR}.${PATCH})
endif()

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(Embree
    REQUIRED_VARS
        EMBREE_INCLUDE_DIR
        EMBREE_LIBRARY
    VERSION_VAR
        EMBREE_VERSION
)
