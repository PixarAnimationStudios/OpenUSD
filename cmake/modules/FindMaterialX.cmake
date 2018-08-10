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
# - Find MaterialX Development Kit
#
# Finds an installed MaterialX Development Kit
#
# Variables that will be defined:
# MATERIALX_FOUND            Defined if MaterialX has been detected
# MATERIALX_BASE_DIR         Path to the root of the MaterialX installation 
# MATERIALX_INCLUDE_DIRS     Path to the MaterialX include directories
# MATERIALX_LIB_DIRS         Path to the MaterialX libraray directories
# MATERIALX_STDLIB_DIR       Path to the MaterialX standard library directory
# MATERIALX_LIBRARIES        List of MaterialX libraries

#
# In:
#  MATERIALX_ROOT
#
# Out:
#  MATERIALX_FOUND
#  MATERIALX_INCLUDE_DIRS
#  MATERIALX_LIB_DIRS
#  MATERIALX_LIBRARIES

find_path(MATERIALX_BASE_DIR
    NAMES
        include/MaterialXCore/Library.h
    HINTS
        "${MATERIALX_ROOT}"
        "$ENV{MATERIALX_ROOT}"
    )

find_path(MATERIALX_INCLUDE_DIRS 
    MaterialXCore/Library.h
    HINTS
        "${MATERIALX_ROOT}"
        "$ENV{MATERIALX_ROOT}"        
        "${MATERIALX_BASE_DIR}"
    PATH_SUFFIXES
        include
    DOC
        "MaterialX Header Path"
)

find_path(MATERIALX_LIB_DIRS 
    libMaterialXCore.a
    HINTS
        "${MATERIALX_ROOT}"
        "$ENV{MATERIALX_ROOT}"        
        "${MATERIALX_BASE_DIR}"
    PATH_SUFFIXES
        lib
    DOC
        "MaterialX Library Path"
)

find_path(MATERIALX_STDLIB_DIR 
    stdlib_defs.mtlx
    HINTS
        "${MATERIALX_ROOT}"
        "$ENV{MATERIALX_ROOT}"        
        "${MATERIALX_BASE_DIR}"
    PATH_SUFFIXES
        documents/Libraries
    DOC
        "MaterialX Standard Libraries Path"
)

foreach(MATERIALX_LIB
    Core
    Format)
    find_library(MATERIALX_${MATERIALX_LIB}_LIBRARY
            MaterialX${MATERIALX_LIB}
        HINTS
            "${MATERIALX_LIB_DIRS}"
        DOC
            "MaterialX's ${MATERIALX_LIB} library path"
        NO_CMAKE_SYSTEM_PATH
    )

    if (MATERIALX_${MATERIALX_LIB}_LIBRARY)
        list(APPEND MATERIALX_LIBRARIES ${MATERIALX_${MATERIALX_LIB}_LIBRARY})
    endif ()
endforeach()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MaterialX
    REQUIRED_VARS
        MATERIALX_BASE_DIR
        MATERIALX_INCLUDE_DIRS
        MATERIALX_LIB_DIRS
        MATERIALX_STDLIB_DIR
)
