
# Copyright Disney Enterprises, Inc.  All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License
# and the following modification to it: Section 6 Trademarks.
# deleted and replaced with:
#
# 6. Trademarks. This License does not grant permission to use the
# trade names, trademarks, service marks, or product names of the
# Licensor and its affiliates, except as required for reproducing
# the content of the NOTICE file.
#
# You may obtain a copy of the License at
# http://www.apache.org/licenses/LICENSE-2.0

option(PTEX_PREFER_STATIC "Prefer static library when linking pTex on linux or osx" OFF)

if (WIN32)
    find_path( PTEX_INCLUDE_DIR
        NAMES
            Ptexture.h
        HINTS
            "${PTEX_LOCATION}/include"
            "$ENV{PTEX_LOCATION}/include"
        PATHS
            "$ENV{PROGRAMFILES}/Ptex/include"
            /usr/include
            DOC "The directory where Ptexture.h resides")
    find_library( PTEX_LIBRARY
        NAMES
            Ptex32 Ptex32s Ptex
        HINTS
            "${PTEX_LOCATION}/lib64"
            "${PTEX_LOCATION}/lib"
            "$ENV{PTEX_LOCATION}/lib64"
            "$ENV{PTEX_LOCATION}/lib"
        PATHS
            "$ENV{PROGRAMFILES}/Ptex/lib"
            /usr/lib64
            /usr/lib
            /usr/lib/w32api
            /usr/local/lib64
            /usr/local/lib
            DOC "The Ptex library")
elseif (APPLE)
    find_path( PTEX_INCLUDE_DIR
        NAMES
            Ptexture.h
        HINTS
            "${PTEX_LOCATION}/include"
            "$ENV{PTEX_LOCATION}/include"
        PATHS
            DOC "The directory where Ptexture.h resides")
    if (${PTEX_PREFER_STATIC})
        set(PTEX_LIBRARY_NAMES libPtex.a Ptex)
    else ()
        set(PTEX_LIBRARY_NAMES Ptex libPtex.a)
    endif ()
    find_library( PTEX_LIBRARY
        NAMES
            ${PTEX_LIBRARY_NAMES}
        PATHS
            "${PTEX_LOCATION}/lib"
            "$ENV{PTEX_LOCATION}/lib"
            DOC "The Ptex Library")
else ()
    find_path( PTEX_INCLUDE_DIR
        NAMES
            Ptexture.h
        HINTS
            "${PTEX_LOCATION}/include"
            "${PTEX_LOCATION}/include/wdas"
            "$ENV{PTEX_LOCATION}/include"
            "$ENV{PTEX_LOCATION}/include/wdas"
        PATHS
            /usr/include
            /usr/local/include
            DOC "The directory where Ptexture.h resides")
    if (${PTEX_PREFER_STATIC})
        set(PTEX_LIBRARY_NAMES libPtex.a libwdasPtex.a Ptex wdasPtex)
    else ()
        set(PTEX_LIBRARY_NAMES Ptex wdasPtex libPtex.a libwdasPtex.a)
    endif ()
    find_library( PTEX_LIBRARY
        NAMES
            ${PTEX_LIBRARY_NAMES}
        HINTS
            "${PTEX_LOCATION}/lib64"
            "${PTEX_LOCATION}/lib"
            "$ENV{PTEX_LOCATION}/lib64"
            "$ENV{PTEX_LOCATION}/lib"
        PATHS
            /usr/lib64
            /usr/lib
            /usr/local/lib64
            /usr/local/lib
            DOC "The Ptex library")
endif ()

if (PTEX_INCLUDE_DIR AND EXISTS "${PTEX_INCLUDE_DIR}/PtexVersion.h" )

    file(STRINGS "${PTEX_INCLUDE_DIR}/PtexVersion.h" TMP REGEX "^#define PtexAPIVersion.*$")
    string(REGEX MATCHALL "[0-9]+" API ${TMP})
    
    file(STRINGS "${PTEX_INCLUDE_DIR}/PtexVersion.h" TMP REGEX "^#define PtexFileMajorVersion.*$")
    string(REGEX MATCHALL "[0-9]+" MAJOR ${TMP})

    file(STRINGS "${PTEX_INCLUDE_DIR}/PtexVersion.h" TMP REGEX "^#define PtexFileMinorVersion.*$")
    string(REGEX MATCHALL "[0-9]+" MINOR ${TMP})

    set(PTEX_VERSION ${API}.${MAJOR}.${MINOR})

elseif (PTEX_INCLUDE_DIR AND EXISTS "${PTEX_INCLUDE_DIR}/Ptexture.h" )

    file(STRINGS "${PTEX_INCLUDE_DIR}/Ptexture.h" TMP REGEX "^#define PtexAPIVersion.*$")
    string(REGEX MATCHALL "[0-9]+" API ${TMP})
    
    file(STRINGS "${PTEX_INCLUDE_DIR}/Ptexture.h" TMP REGEX "^#define PtexFileMajorVersion.*$")
    string(REGEX MATCHALL "[0-9]+" MAJOR ${TMP})

    file(STRINGS "${PTEX_INCLUDE_DIR}/Ptexture.h" TMP REGEX "^#define PtexFileMinorVersion.*$")
    string(REGEX MATCHALL "[0-9]+" MINOR ${TMP})

    set(PTEX_VERSION ${API}.${MAJOR}.${MINOR})

endif()

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(PTex
    REQUIRED_VARS
        PTEX_INCLUDE_DIR
        PTEX_LIBRARY
    VERSION_VAR
        PTEX_VERSION
)

if (PTEX_FOUND)
    set(PTEX_LIBRARIES ${PTEX_LIBRARY})
endif(PTEX_FOUND)

mark_as_advanced(
  PTEX_INCLUDE_DIR
  PTEX_LIBRARY
)