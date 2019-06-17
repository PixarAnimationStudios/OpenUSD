#
# Copyright 2018 Pixar
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
#   RENDERMAN_INCLUDE_DIR - path to renderman header directory
#   RENDERMAN_LIBRARY     - path to renderman library files
#       RENDERMAN_FOUND   - true if renderman was found
#   RENDERMAN_VERSION_MAJOR - major version of renderman found
#   RENDERMAN_VERSION_MINOR - minor version of renderman found
#
# Example usage:
#   find_package(RENDERMAN)
#   if(RENDERMAN_FOUND)
#     message("RENDERMAN found: ${RENDERMAN_LIBRARY}")
#   endif()
#
#=============================================================================

# Use libloadprman.a to handle runtime loading of prman.
if(WIN32)
    set (RENDERMAN_LIB_NAME libloadprman.lib)
else()
    set (RENDERMAN_LIB_NAME libloadprman.a)
endif()

find_library(RENDERMAN_LIBRARY
        "${RENDERMAN_LIB_NAME}"
    HINTS
        "${RENDERMAN_LOCATION}/lib64"
        "${RENDERMAN_LOCATION}/lib"
        "$ENV{RENDERMAN_LOCATION}/lib64"
        "$ENV{RENDERMAN_LOCATION}/lib"
        "$ENV{RMANTREE}/lib"
        "$ENV{RMANTREE}/lib64"
    DOC
        "Renderman library path"
)

find_path(RENDERMAN_INCLUDE_DIR
    RixInterfaces.h
    HINTS
        "${RENDERMAN_LOCATION}/include"
        "$ENV{RENDERMAN_LOCATION}/include"
        "$ENV{RMANTREE}/include"
    DOC
        "Renderman headers path"
)

# Parse version
if (RENDERMAN_INCLUDE_DIR AND EXISTS "${RENDERMAN_INCLUDE_DIR}/RixInterfaces.h" )
    file(STRINGS "${RENDERMAN_INCLUDE_DIR}/prmanapi.h" TMP REGEX "^#define _PRMANAPI_VERSION_MAJOR_.*$")
    string(REGEX MATCHALL "[0-9]+" MAJOR ${TMP})
    file(STRINGS "${RENDERMAN_INCLUDE_DIR}/prmanapi.h" TMP REGEX "^#define _PRMANAPI_VERSION_MINOR_.*$")
    string(REGEX MATCHALL "[0-9]+" MINOR ${TMP})
    file(STRINGS "${RENDERMAN_INCLUDE_DIR}/prmanapi.h" TMP REGEX "^#define _PRMANAPI_VERSION_BUILD_.*$")
    string(REGEX MATCHALL "[0-9]+" PATCH ${TMP})

    set (RENDERMAN_VERSION ${MAJOR}.${MINOR}.${PATCH})
    set (RENDERMAN_VERSION_MAJOR ${MAJOR})
    set (RENDERMAN_VERSION_MINOR ${MINOR})
endif()

# will set RENDERMAN_FOUND
include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(Renderman
    REQUIRED_VARS
        RENDERMAN_INCLUDE_DIR
        RENDERMAN_LIBRARY
        RENDERMAN_VERSION_MAJOR
        RENDERMAN_VERSION_MINOR
    VERSION_VAR
        RENDERMAN_VERSION
)
