#
# Copyright 2022 Apple
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


# FindImath
# -------
#
# Find Imath include directories and libraries.
#
# Input Variables:
#
# * Imath_ROOT : The Root location of the built Imath project, where we should look for the headers and dylibs
# * Imath_USE_DEBUG_BUILD : Use the Debug libraries instead of the release library
#
# Output Variables:
# * FOUND_Imath : Set if the project is found
# * Imath_LIBRARY | Imath_LIBRARIES : The (list of) Imath libraries
# * Imath_INCLUDE_DIR | Imath_INCLUDE_DIRS : The location(s) that include the Imath headers

if(NOT DEFINED Imath_USE_DEBUG_BUILD)
    if(CMAKE_BUILD_TYPE MATCHES "(Debug|DEBUG|debug|RelWithDebInfo|RELWITHDEBINFO|relwithdebinfo)")
        set(Imath_USE_DEBUG_BUILD TRUE)
    else()
        set(Imath_USE_DEBUG_BUILD FALSE)
    endif()
endif()

find_path(Imath_INCLUDE_DIR
        Imath/half.h
        HINTS
        "${Imath_LOCATION}"
        "$ENV{IMATH_LOCATION}"
        "${Imath_ROOT}"
        PATH_SUFFIXES
        include/
        DOC
        "Imath headers path"
        )
list(APPEND Imath_INCLUDE_DIRS ${Imath_INCLUDE_DIR})
list(APPEND Imath_INCLUDE_DIRS ${Imath_INCLUDE_DIR}/Imath)

if(Imath_USE_DEBUG_BUILD)
    find_library(Imath_LIBRARY
            NAMES
            Imath-${Imath_MAJOR_VERSION}_${Imath_MINOR_VERSION}_d
            Imath_d
            HINTS
            "${Imath_LOCATION}"
            "$ENV{Imath_LOCATION}"
            "${Imath_ROOT}"
            PATH_SUFFIXES
            lib/
            DOC
            "Imath library path"
            )
    if(Imath_LIBRARY)
        SET(Imath_FOUND TRUE)
        list(APPEND Imath_LIBRARIES ${Imath_LIBRARY})
    else()
        SET(Imath_FOUND FALSE)
        MESSAGE(WARNING "Could not find Debug Imath library")
    endif()
endif()

if (NOT Imath_FOUND)
    find_library(Imath_LIBRARY
            NAMES
            Imath-${Imath_MAJOR_VERSION}_${Imath_MINOR_VERSION}
            Imath
            HINTS
            "${Imath_LOCATION}"
            "$ENV{Imath_LOCATION}"
            "${Imath_ROOT}"
            PATH_SUFFIXES
            lib/
            DOC
            "Imath library path"
            )
    if(Imath_LIBRARY)
        SET(Imath_FOUND TRUE)
        list(APPEND Imath_LIBRARIES ${Imath_LIBRARY})
    else()
        SET(Imath_FOUND FALSE)
    endif()
endif()

