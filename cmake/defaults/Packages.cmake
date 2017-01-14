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
# Core USD Package Requirements 
# ----------------------------------------------
# --Python.  We are generally but not completely 2.6 compliant.
find_package(PythonInterp 2.7 REQUIRED)
find_package(PythonLibs 2.7 REQUIRED)

# --Boost
find_package(Boost
    COMPONENTS
        date_time
        iostreams
        program_options
        python
        regex
        system
    REQUIRED
)
# --Double Conversion
find_package(DoubleConversion REQUIRED)

# --TBB
find_package(TBB REQUIRED)
add_definitions(${TBB_DEFINITIONS})

# --OpenEXR
find_package(OpenEXR REQUIRED)

# --pthread
find_package(Threads REQUIRED)

# --math
if(WIN32)
    # Math functions are linked automatically by including math.h on Windows.
    set(M_LIB "")
else()
    find_library(M_LIB m)
endif()

# --Jinja2
find_package(Jinja2)

if (NOT PXR_MALLOC_LIBRARY)
    if (NOT WIN32)
        message(STATUS "Using default system allocator because PXR_MALLOC_LIBRARY is unspecified")
    endif()
endif()

# Developer Options Package Requirements
# ----------------------------------------------
if (PXR_VALIDATE_GENERATED_CODE)
    find_package(BISON 2.4.1 EXACT)
    # Flex 2.5.39+ is required, generated API is generated incorrectly in
    # 2.5.35, at least. scan_bytes generates with (..., int len, ...) instead of
    # the correct (..., yy_size_t len, ...).  Lower at your own peril.
    find_package(FLEX 2.5.39 EXACT)
endif()


# Imaging Components Package Requirements
# ----------------------------------------------

if (PXR_BUILD_IMAGING)
    # --OpenImageIO
    find_package(OpenImageIO REQUIRED)
    # --OpenGL
    find_package(OpenGL REQUIRED)
    find_package(GLEW REQUIRED)
    # --Opensubdiv
    find_package(OpenSubdiv 3 REQUIRED)
    # --Ptex
    find_package(PTex REQUIRED)
    # --X11
    if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
        find_package(X11)
    endif()
    # --PySide
    find_package(PySide)
    # --PyOpenGL
    find_package(PyOpenGL)
endif()

# Third Party Plugin Package Requirements
# ----------------------------------------------
if (PXR_BUILD_KATANA_PLUGIN)
    find_package(KatanaAPI REQUIRED)
    find_package(Boost
        COMPONENTS
        thread
        REQUIRED
        )
endif()

if (PXR_BUILD_MAYA_PLUGIN)
    find_package(Maya REQUIRED)
    find_package(GLUT REQUIRED)
endif()

if (PXR_BUILD_ALEMBIC_PLUGIN)
    find_package(Alembic REQUIRED)
    find_package(HDF5 REQUIRED
        COMPONENTS
            HL
        REQUIRED
    )
endif()
