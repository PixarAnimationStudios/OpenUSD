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

# Save the current value of BUILD_SHARED_LIBS and restore it at
# the end of this file, since some of the Find* modules invoked
# below may wind up stomping over this value.
set(build_shared_libs "${BUILD_SHARED_LIBS}")

# Core USD Package Requirements 
# ----------------------------------------------

# Threads.  Save the libraries needed in PXR_THREAD_LIBS;  we may modify
# them later.  We need the threads package because some platforms require
# it when using C++ functions from #include <thread>.
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
find_package(Threads REQUIRED)
set(PXR_THREAD_LIBS "${CMAKE_THREAD_LIBS_INIT}")

if(PXR_ENABLE_PYTHON_SUPPORT)
    # --Python.
    if(PXR_USE_PYTHON_3)
        find_package(PythonInterp 3.0 REQUIRED)
        find_package(PythonLibs 3.0 REQUIRED)
    else()
        find_package(PythonInterp 2.7 REQUIRED)
        find_package(PythonLibs 2.7 REQUIRED)
    endif()

    find_package(Boost
        COMPONENTS
            program_options
        REQUIRED
    )

    if (((${Boost_VERSION_STRING} VERSION_GREATER_EQUAL "1.67") AND
         (${Boost_VERSION_STRING} VERSION_LESS "1.70")) OR
        ((${Boost_VERSION_STRING} VERSION_GREATER_EQUAL "1.70") AND
          Boost_NO_BOOST_CMAKE))
        # As of boost 1.67 the boost_python component name includes the
        # associated Python version (e.g. python27, python36). After boost 1.70
        # the built-in cmake files will deal with this. If we are using boost
        # that does not have working cmake files, or we are using a new boost
        # and not using cmake's boost files, we need to do the below.
        #
        # Find the component under the versioned name and then set the generic
        # Boost_PYTHON_LIBRARY variable so that we don't have to duplicate this
        # logic in each library's CMakeLists.txt.
        set(python_version_nodot "${PYTHON_VERSION_MAJOR}${PYTHON_VERSION_MINOR}")
        find_package(Boost
            COMPONENTS
                python${python_version_nodot}
            REQUIRED
        )
        set(Boost_PYTHON_LIBRARY "${Boost_PYTHON${python_version_nodot}_LIBRARY}")
    else()
        find_package(Boost
            COMPONENTS
                python
            REQUIRED
        )
    endif()

    # --Jinja2
    find_package(Jinja2)
else()
    find_package(PythonInterp 2.7 REQUIRED)
 
    # --Boost
    find_package(Boost
        COMPONENTS
            program_options
        REQUIRED
    )
endif()

# --TBB
find_package(TBB REQUIRED COMPONENTS tbb)
add_definitions(${TBB_DEFINITIONS})

# --math
if(WIN32)
    # Math functions are linked automatically by including math.h on Windows.
    set(M_LIB "")
else()
    find_library(M_LIB m)
endif()

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
    if (PXR_BUILD_OPENIMAGEIO_PLUGIN)
        find_package(OpenEXR REQUIRED)
        find_package(OpenImageIO REQUIRED)
        add_definitions(-DPXR_OIIO_PLUGIN_ENABLED)
    endif()
    # --OpenColorIO
    if (PXR_BUILD_OPENCOLORIO_PLUGIN)
        find_package(OpenColorIO REQUIRED)
        add_definitions(-DPXR_OCIO_PLUGIN_ENABLED)
    endif()
    # --OpenGL
    if (PXR_ENABLE_GL_SUPPORT)
        # Prefer legacy GL library over GLVND libraries if both
        # are installed.
        if (POLICY CMP0072)
            cmake_policy(SET CMP0072 OLD)
        endif()
        find_package(OpenGL REQUIRED)
        find_package(GLEW REQUIRED)
    endif()
    # --Opensubdiv
    set(OPENSUBDIV_USE_GPU ${PXR_ENABLE_GL_SUPPORT})
    find_package(OpenSubdiv 3 REQUIRED)
    # --Ptex
    if (PXR_ENABLE_PTEX_SUPPORT)
        find_package(PTex REQUIRED)
        add_definitions(-DPXR_PTEX_SUPPORT_ENABLED)
    endif()
    # --OpenVDB
    if (PXR_ENABLE_OPENVDB_SUPPORT)
        find_package(OpenEXR REQUIRED)
        find_package(OpenVDB REQUIRED)
        add_definitions(-DPXR_OPENVDB_SUPPORT_ENABLED)
    endif()
    # --X11
    if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
        find_package(X11)
    endif()
    # --Embree
    if (PXR_BUILD_EMBREE_PLUGIN)
        find_package(Embree REQUIRED)
    endif()
endif()

if (PXR_BUILD_USDVIEW)
    # --PySide
    find_package(PySide REQUIRED)
    # --PyOpenGL
    find_package(PyOpenGL REQUIRED)
endif()

# Third Party Plugin Package Requirements
# ----------------------------------------------
if (PXR_BUILD_PRMAN_PLUGIN)
    find_package(Renderman REQUIRED)
endif()

if (PXR_BUILD_ALEMBIC_PLUGIN)
    find_package(Alembic REQUIRED)
    find_package(OpenEXR REQUIRED)
    if (PXR_ENABLE_HDF5_SUPPORT)
        find_package(HDF5 REQUIRED
            COMPONENTS
                HL
            REQUIRED
        )
    endif()
endif()

if (PXR_BUILD_DRACO_PLUGIN)
    find_package(Draco REQUIRED)
endif()

if (PXR_BUILD_MATERIALX_PLUGIN)
    find_package(MaterialX REQUIRED)
endif()

if(PXR_ENABLE_OSL_SUPPORT)
    find_package(OSL REQUIRED)
    find_package(OpenEXR REQUIRED)
endif()

# ----------------------------------------------

set(BUILD_SHARED_LIBS "${build_shared_libs}")
