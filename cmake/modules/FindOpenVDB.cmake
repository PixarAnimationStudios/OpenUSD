#
# Copyright 2019 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

# Find OpenVDB header.
find_path( OPENVDB_INCLUDE_DIR
    NAMES
        openvdb/openvdb.h
    PATH_SUFFIXES
        include/
    HINTS
        "${OPENVDB_LOCATION}"
        "$ENV{OPENVDB_LOCATION}"
    DOC
        "OpenVDB headers path"
)

find_library( OPENVDB_LIBRARY
    NAMES
        openvdb
    PATH_SUFFIXES
        lib/
    HINTS
        ${OPENVDB_LOCATION}
        $ENV{OPENVDB_LOCATION}
    DOC
        "The OpenVDB library path"
)

if (OPENVDB_INCLUDE_DIR AND EXISTS "${OPENVDB_INCLUDE_DIR}/openvdb/version.h")
    file(STRINGS "${OPENVDB_INCLUDE_DIR}/openvdb/version.h" TMP REGEX "^#define OPENVDB_LIBRARY_MAJOR_VERSION_NUMBER.*$")
    string(REGEX MATCHALL "[0-9]+" MAJOR ${TMP})
    file(STRINGS "${OPENVDB_INCLUDE_DIR}/openvdb/version.h" TMP REGEX "^#define OPENVDB_LIBRARY_MINOR_VERSION_NUMBER.*$")
    string(REGEX MATCHALL "[0-9]+" MINOR ${TMP})
    file(STRINGS "${OPENVDB_INCLUDE_DIR}/openvdb/version.h" TMP REGEX "^#define OPENVDB_LIBRARY_PATCH_VERSION_NUMBER.*$")
    string(REGEX MATCHALL "[0-9]+" PATCH ${TMP})
    set(OPENVDB_VERSION ${MAJOR}.${MINOR}.${PATCH})
endif()

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(OpenVDB
    REQUIRED_VARS
        OPENVDB_INCLUDE_DIR
        OPENVDB_LIBRARY
    VERSION_VAR
        OPENVDB_VERSION
)
