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

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(OpenVDB
    DEFAULT_MSG
    OPENVDB_INCLUDE_DIR
    OPENVDB_LIBRARY
)
