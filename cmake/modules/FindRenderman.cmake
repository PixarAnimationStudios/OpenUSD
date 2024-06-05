#
# Copyright 2018 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
#=============================================================================
#
# The module defines the following variables:
#   RENDERMAN_INCLUDE_DIR - path to renderman header directory
#   RENDERMAN_LIBRARY     - path to renderman library files
#   RENDERMAN_EXECUTABLE  - path the prman executable
#   RENDERMAN_BINARY_DIR  - path to the renderman binary directory
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
    set (LOADPRMAN_LIB_NAME libloadprman.lib)
    set (PRMAN_LIB_NAME libprman.lib)
    set (PRMAN_STATS_LIB_NAME libstats.lib)
    set (PXRCORE_LIB_NAME libpxrcore.lib)
elseif(APPLE)
    set (LOADPRMAN_LIB_NAME libloadprman.a)
    set (PRMAN_LIB_NAME libprman.dylib)
    set (PRMAN_STATS_LIB_NAME libstats.dylib)
    set (PXRCORE_LIB_NAME libpxrcore.dylib)
elseif(UNIX)
    set (LOADPRMAN_LIB_NAME libloadprman.a)
    set (PRMAN_LIB_NAME libprman.so)
    set (PRMAN_STATS_LIB_NAME libstats.so)
    set (PXRCORE_LIB_NAME libpxrcore.so)
endif()

find_library(LOADPRMAN_LIBRARY
        "${LOADPRMAN_LIB_NAME}"
    HINTS
        "${RENDERMAN_LOCATION}/lib64"
        "${RENDERMAN_LOCATION}/lib"
        "$ENV{RENDERMAN_LOCATION}/lib64"
        "$ENV{RENDERMAN_LOCATION}/lib"
        "$ENV{RMANTREE}/lib"
        "$ENV{RMANTREE}/lib64"
    DOC
        "Load Renderman library path"
)

find_library(PRMAN_LIBRARY
    "${PRMAN_LIB_NAME}"
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

find_library(PRMAN_STATS_LIBRARY
    "${PRMAN_STATS_LIB_NAME}"
    HINTS
        "${RENDERMAN_LOCATION}/lib64"
        "${RENDERMAN_LOCATION}/lib"
        "$ENV{RENDERMAN_LOCATION}/lib64"
        "$ENV{RENDERMAN_LOCATION}/lib"
        "$ENV{RMANTREE}/lib"
        "$ENV{RMANTREE}/lib64"
    DOC
        "Renderman statistics library path"
)

find_library(PXRCORE_LIBRARY
    "${PXRCORE_LIB_NAME}"
    HINTS
        "${RENDERMAN_LOCATION}/lib64"
        "${RENDERMAN_LOCATION}/lib"
        "$ENV{RENDERMAN_LOCATION}/lib64"
        "$ENV{RENDERMAN_LOCATION}/lib"
        "$ENV{RMANTREE}/lib"
        "$ENV{RMANTREE}/lib64"
    DOC
        "Renderman core library path"
)

find_path(RENDERMAN_INCLUDE_DIR
    prmanapi.h
    HINTS
        "${RENDERMAN_LOCATION}/include"
        "$ENV{RENDERMAN_LOCATION}/include"
        "$ENV{RMANTREE}/include"
    DOC
        "Renderman headers path"
)

find_program(RENDERMAN_EXECUTABLE
    prman
    HINTS
        "${RENDERMAN_LOCATION}/bin"
        "$ENV{RENDERMAN_LOCATION}/bin"
        "$ENV{RMANTREE}/bin"
    DOC
        "Renderman prman executable path"
)

get_filename_component(RENDERMAN_BINARY_DIR
    ${RENDERMAN_EXECUTABLE}
    PATH)

# Parse version
if (RENDERMAN_INCLUDE_DIR AND EXISTS "${RENDERMAN_INCLUDE_DIR}/prmanapi.h" )
    file(STRINGS "${RENDERMAN_INCLUDE_DIR}/prmanapi.h" TMP REGEX "^#define _PRMANAPI_VERSION_MAJOR_.*$")
    string(REGEX MATCHALL "[0-9]+" MAJOR ${TMP})

    set (RENDERMAN_VERSION_MAJOR ${MAJOR})
endif()

# will set RENDERMAN_FOUND
include(FindPackageHandleStandardArgs)

set(required_vars "")
list(APPEND required_vars "RENDERMAN_INCLUDE_DIR")
list(APPEND required_vars "RENDERMAN_EXECUTABLE")
list(APPEND required_vars "RENDERMAN_BINARY_DIR")
list(APPEND required_vars "RENDERMAN_VERSION_MAJOR")
list(APPEND required_vars "PRMAN_LIBRARY")
list(APPEND required_vars "PRMAN_STATS_LIBRARY")
list(APPEND required_vars "PXRCORE_LIBRARY")

find_package_handle_standard_args(Renderman
  REQUIRED_VARS
      ${required_vars}
)

