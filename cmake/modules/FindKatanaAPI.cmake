#
# Copyright 2016 Pixar
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
# - Find Katana API
#
# Finds an installed Katana plugin API
#
# Variables that will be defined:
# KATANA_API_FOUND         Defined if a Katana installation has been detected
# KATANA_API_INCLUDE_DIR   Path to the Katana API include directories
# KATANA_API_SOURCE_DIR    Path to the Katana API source directories
# KATANA_API_VERSION       Katana API version

if(WIN32)
    set(KATANA_EXECUTABLE bin/katanaBin.exe)
else()
    set(KATANA_EXECUTABLE katana)
endif()

find_path(KATANA_API_BASE_DIR
    NAMES
        ${KATANA_EXECUTABLE}
    HINTS
        "${KATANA_API_LOCATION}"
        "$ENV{KATANA_API_LOCATION}"
)

set(KATANA_API_HEADER FnAPI/FnAPI.h)

find_path(KATANA_API_INCLUDE_DIR 
    ${KATANA_API_HEADER}
    HINTS
        "${KATANA_API_LOCATION}"
        "$ENV{KATANA_API_LOCATION}"
        "${KATANA_API_BASE_DIR}"
    PATH_SUFFIXES
        plugin_apis/include/
    DOC
        "Katana plugin API headers path"
)

if(KATANA_API_INCLUDE_DIR AND EXISTS "${KATANA_API_INCLUDE_DIR}/${KATANA_API_HEADER}")
    foreach(comp MAJOR MINOR RELEASE)
        file(STRINGS
            ${KATANA_API_INCLUDE_DIR}/${KATANA_API_HEADER}
            TMP
            REGEX "#define KATANA_VERSION_${comp} .*$")
        string(REGEX MATCHALL "[0-9]+" KATANA_API_${comp}_VERSION ${TMP})
    endforeach()
    set(KATANA_API_VERSION ${KATANA_API_MAJOR_VERSION}.${KATANA_API_MINOR_VERSION}.${KATANA_API_RELEASE_VERSION})
endif()

# Sub-system API versions
if(KATANA_API_INCLUDE_DIR)
    if (EXISTS "${KATANA_API_INCLUDE_DIR}/FnAsset/plugin")
        set(KATANA_API_ASSET_API_VERSION 2)
    else()
        set(KATANA_API_ASSET_API_VERSION 1)
    endif()
    if (EXISTS "${KATANA_API_INCLUDE_DIR}/FnAttributeModifier/plugin")
        set(KATANA_API_AMP_API_VERSION 2)
    else()
        set(KATANA_API_AMP_API_VERSION 1)
    endif()
    if (EXISTS "${KATANA_API_INCLUDE_DIR}/FnSceneGraphGenerator/plugin")
        set(KATANA_API_SGG_API_VERSION 2)
    else()
        set(KATANA_API_SGG_API_VERSION 1)
    endif()
    if (EXISTS "${KATANA_API_INCLUDE_DIR}/FnRender/plugin")
        set(KATANA_API_RENDER_API_VERSION 2)
    else()
        set(KATANA_API_RENDER_API_VERSION 1)
    endif()
    if (EXISTS "${KATANA_API_INCLUDE_DIR}/FnViewerModifier/plugin")
        set(KATANA_API_VMP_API_VERSION 2)
    else()
        set(KATANA_API_VMP_API_VERSION 1)
    endif()
    if (EXISTS "${KATANA_API_INCLUDE_DIR}/FnGeolib")
        set(KATANA_API_OP_API_VERSION 2)
    else()
        set(KATANA_API_OP_API_VERSION 1)
    endif()
    if (EXISTS "${KATANA_API_INCLUDE_DIR}/FnAttributeFunction")
        set(KATANA_API_ATTRFNC_API_VERSION 2)
    else()
        set(KATANA_API_ATTRFNC_API_VERSION 1)
    endif()
endif()

find_path(KATANA_API_SOURCE_DIR 
    FnConfig/FnConfig.cpp
    HINTS
        "${KATANA_API_LOCATION}"
        "$ENV{KATANA_API_LOCATION}"
        "${KATANA_API_BASE_DIR}"
    PATH_SUFFIXES
        plugin_apis/src/
    DOC
        "Katana plugin API source path"
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Katana
    REQUIRED_VARS
        KATANA_API_BASE_DIR
        KATANA_API_INCLUDE_DIR
        KATANA_API_SOURCE_DIR
    VERSION_VAR
        KATANA_API_VERSION
)

set(KATANA_API_INCLUDE_DIRS ${KATANA_API_INCLUDE_DIR})

