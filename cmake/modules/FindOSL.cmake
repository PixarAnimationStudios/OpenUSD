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

# Find OSL header.
find_path(OSL_INCLUDE_DIR
    NAMES
        OSL/oslversion.h
    PATH_SUFFIXES
        include/
    HINTS
        "${OSL_LOCATION}"
        "$ENV{OSL_LOCATION}"
    DOC
        "OSL headers path"
    )

# Parse OSL version.
if(OSL_INCLUDE_DIR)
    set(osl_config_file "${OSL_INCLUDE_DIR}/OSL/oslversion.h")
    if(EXISTS ${osl_config_file})
        file(STRINGS
                ${osl_config_file}
                TMP
                REGEX "#define OSL_LIBRARY_VERSION_MAJOR.*$")
        string(REGEX MATCHALL "[0-9]" OSL_MAJOR_VERSION ${TMP})
        file(STRINGS
                ${osl_config_file}
                TMP
                REGEX "#define OSL_LIBRARY_VERSION_MINOR.*$")
        string(REGEX MATCHALL "[0-9]" OSL_MINOR_VERSION ${TMP})
    endif()
endif()

# Find libraries and binaries
find_library (OSL_EXEC_LIBRARY
    NAMES
        oslexec
    PATH_SUFFIXES
        lib/
    HINTS
        "${OSL_LOCATION}"
        "$ENV{OSL_LOCATION}"
    )
find_library (OSL_COMP_LIBRARY
    NAMES
        oslcomp
    PATH_SUFFIXES
        lib/
    HINTS
        "${OSL_LOCATION}"
        "$ENV{OSL_LOCATION}"
    )
find_library (OSL_QUERY_LIBRARY
    NAMES
        oslquery
    PATH_SUFFIXES
        lib/
    HINTS
        "${OSL_LOCATION}"
        "$ENV{OSL_LOCATION}"
    )
find_program (OSL_OSLC_EXECUTABLE
    NAMES
        oslc
    PATH_SUFFIXES
        bin/
    HINTS
        "${OSL_LOCATION}"
        "$ENV{OSL_LOCATION}"
    )
find_program (OSL_OSLINFO_EXECUTABLE
    NAMES
        oslinfo
    PATH_SUFFIXES
        bin/
    HINTS
        "${OSL_LOCATION}"
        "$ENV{OSL_LOCATION}"
    )

include (FindPackageHandleStandardArgs)
find_package_handle_standard_args (OSL
        DEFAULT_MSG
        OSL_INCLUDE_DIR
        OSL_EXEC_LIBRARY
        OSL_COMP_LIBRARY
        OSL_QUERY_LIBRARY
        OSL_OSLC_EXECUTABLE
        OSL_OSLINFO_EXECUTABLE
    )
