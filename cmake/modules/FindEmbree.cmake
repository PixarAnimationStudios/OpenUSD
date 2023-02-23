#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
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

find_path(EMBREE_INCLUDE_DIR
    embree4/rtcore.h
    embree3/rtcore.h
HINTS
    "${EMBREE_LOCATION}/include"
    "$ENV{EMBREE_LOCATION}/include"
DOC
    "Embree headers path"
)

if (EMBREE_INCLUDE_DIR AND EXISTS "${EMBREE_INCLUDE_DIR}/embree4/rtcore.h")
    set(EMBREE_VERSIONED_LIBRARY_NAME "embree4")
    set(EMBREE_VERSION_HEADER_FILE_NAME "rtcore_config.h")
else()
    set(EMBREE_VERSIONED_LIBRARY_NAME "embree3")
    set(EMBREE_VERSION_HEADER_FILE_NAME "rtcore_version.h")
endif()

if (APPLE)
    set (EMBREE_LIB_NAME "lib${EMBREE_VERSIONED_LIBRARY_NAME}.dylib")
elseif (UNIX)
    set (EMBREE_LIB_NAME "lib${EMBREE_VERSIONED_LIBRARY_NAME}.so")
elseif (WIN32)
    set (EMBREE_LIB_NAME "${EMBREE_VERSIONED_LIBRARY_NAME}.lib")
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

set(EMBREE_VERSION_HEADER_FILE_PATH "${EMBREE_INCLUDE_DIR}/${EMBREE_VERSIONED_LIBRARY_NAME}/${EMBREE_VERSION_HEADER_FILE_NAME}")

if (EMBREE_INCLUDE_DIR AND EXISTS "${EMBREE_VERSION_HEADER_FILE_PATH}")
    file(STRINGS "${EMBREE_VERSION_HEADER_FILE_PATH}" TMP REGEX "^#define RTC_VERSION_MAJOR.*$")
    string(REGEX MATCHALL "[0-9]+" MAJOR ${TMP})
    file(STRINGS "${EMBREE_VERSION_HEADER_FILE_PATH}" TMP REGEX "^#define RTC_VERSION_MINOR.*$")
    string(REGEX MATCHALL "[0-9]+" MINOR ${TMP})
    file(STRINGS "${EMBREE_VERSION_HEADER_FILE_PATH}" TMP REGEX "^#define RTC_VERSION_PATCH.*$")
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
