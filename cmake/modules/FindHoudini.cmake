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
# - Find Houdini Development Kit
#
# Finds an installed Houdini Development Kit
#
# Variables that will be defined:
# HOUDINI_FOUND            Defined if HDK installation has been detected
# HOUDINI_BASE_DIR         Path to the root of the Houdini installation 
# HOUDINI_INCLUDE_DIRS      Path to the HDK include directories
# HOUDINI_LIB_DIRS          Path to the HDK libraray directories
# HOUDINI_VERSION           Full Houdini version, 16.0.596 for example
# HOUDINI_MAJOR_VERSION
# HOUDINI_MINOR_VERSION     
# HOUDINI_BUILD_VERSION     

#
# In:
#  HOUDINI_ROOT
#
# Out:
#  HOUDINI_FOUND
#  HOUDINI_VERSION
#  HOUDINI_BUILD_VERSION
#  HOUDINI_INCLUDE_DIRS
#  HOUDINI_LIBRARY_DIRS
#  HOUDINI_LIBRARIES
#  HOUDINI_APPS1_LIBRARY
#  HOUDINI_APPS2_LIBRARY
#  HOUDINI_APPS3_LIBRARY
#  HOUDINI_DEVICE_LIBRARY
#  HOUDINI_GEO_LIBRARY
#  HOUDINI_OP1_LIBRARY
#  HOUDINI_OP2_LIBRARY
#  HOUDINI_OP3_LIBRARY
#  HOUDINI_OPZ_LIBRARY
#  HOUDINI_PRM_LIBRARY
#  HOUDINI_RAY_LIBRARY
#  HOUDINI_SIM_LIBRARY
#  HOUDINI_UI_LIBRARY
#  HOUDINI_UT_LIBRARY

find_path(HOUDINI_BASE_DIR
    NAMES
        houdini
    HINTS
        "${HOUDINI_ROOT}"
        "$ENV{HOUDINI_ROOT}"
    )

find_path(HOUDINI_INCLUDE_DIRS 
    UT/UT_Version.h
    HINTS
        "${HOUDINI_ROOT}"
        "$ENV{HOUDINI_ROOT}"        
        "${HOUDINI_BASE_DIR}"
    PATH_SUFFIXES
        toolkit/include/
    DOC
        "Houdini Development Kit Header Path"
)

if (UNIX)
    set(HOUDINI_LIB_NAME "libHoudiniGEO.so")
    set(HOUDINI_LIB_PATH_SUFFIX "dsolib/")
elseif(WIN32)
    set(HOUDINI_LIB_NAME "libGEO.lib")
    set(HOUDINI_LIB_PATH_SUFFIX "custom/houdini/dsolib/")
endif()

find_path(HOUDINI_LIB_DIRS 
    ${HOUDINI_LIB_NAME}
    HINTS
        "${HOUDINI_ROOT}"
        "$ENV{HOUDINI_ROOT}"        
        "${HOUDINI_BASE_DIR}"
    PATH_SUFFIXES
        ${HOUDINI_LIB_PATH_SUFFIX}
    DOC
        "Houdini Development Kit Library Path"
)

foreach(HOUDINI_LIB
    APPS1
    APPS2
    APPS3
    DEVICE
    GEO
    OP1
    OP2
    OP3
    OPZ
    PRM
    RAY
    SIM
    UI
    UT)
    find_library(HOUDINI_${HOUDINI_LIB}_LIBRARY
            Houdini${HOUDINI_LIB}
        HINTS
            "${HOUDINI_LIB_DIRS}"
        DOC
            "Houdini's ${HOUDINI_LIB} library path"
        NO_CMAKE_SYSTEM_PATH
    )

    if (HOUDINI_${HOUDINI_LIB}_LIBRARY)
        list(APPEND HOUDINI_LIBRARIES ${HOUDINI_${HOUDINI_LIB}_LIBRARY})
    endif ()
endforeach()

if(HOUDINI_INCLUDE_DIRS AND EXISTS "${HOUDINI_INCLUDE_DIRS}/SYS/SYS_Version.h")
    foreach(comp FULL MAJOR MINOR BUILD)
        file(STRINGS
            ${HOUDINI_INCLUDE_DIRS}/SYS/SYS_Version.h
            TMP
            REGEX "#define SYS_VERSION_${comp} .*$")
        string(REGEX MATCHALL "[0-9.]+" HOUDINI_${comp}_VERSION ${TMP})
    endforeach()
endif()

set(HOUDINI_VERSION ${HOUDINI_FULL_VERSION})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Houdini
    REQUIRED_VARS
        HOUDINI_BASE_DIR
        HOUDINI_INCLUDE_DIRS
        HOUDINI_LIB_DIRS
    VERSION_VAR
        HOUDINI_VERSION
)



