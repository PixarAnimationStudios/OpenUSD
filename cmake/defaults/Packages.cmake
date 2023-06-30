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

# Find Boost package before getting any boost specific components as we need to
# disable boost-provided cmake config, based on the boost version found.
find_package(Boost REQUIRED)

# Boost provided cmake files (introduced in boost version 1.70) result in 
# inconsistent build failures on different platforms, when trying to find boost 
# component dependencies like python, etc. Refer some related
# discussions:
# https://github.com/boostorg/python/issues/262#issuecomment-483069294
# https://github.com/boostorg/boost_install/issues/12#issuecomment-508683006
#
# Hence to avoid issues with Boost provided cmake config, Boost_NO_BOOST_CMAKE
# is enabled by default for boost version 1.70 and above. If a user explicitly 
# set Boost_NO_BOOST_CMAKE to Off, following will be a no-op.
option(Boost_NO_BOOST_CMAKE "Disable boost-provided cmake config" ON)
if (Boost_NO_BOOST_CMAKE)
    message(STATUS "Disabling boost-provided cmake config")
endif()

if(PXR_ENABLE_PYTHON_SUPPORT)
    # 1--Python.
    macro(setup_python_package package)
        find_package(${package} COMPONENTS Interpreter Development REQUIRED)

        # Set up versionless variables so that downstream libraries don't
        # have to worry about which Python version is being used.
        set(PYTHON_EXECUTABLE "${${package}_EXECUTABLE}")
        set(PYTHON_INCLUDE_DIRS "${${package}_INCLUDE_DIRS}")
        set(PYTHON_VERSION_MAJOR "${${package}_VERSION_MAJOR}")
        set(PYTHON_VERSION_MINOR "${${package}_VERSION_MINOR}")

        # Convert paths to CMake path format on Windows to avoid string parsing
        # issues when we pass PYTHON_EXECUTABLE or PYTHON_INCLUDE_DIRS to
        # pxr_library or other functions.
        if(WIN32)
            file(TO_CMAKE_PATH ${PYTHON_EXECUTABLE} PYTHON_EXECUTABLE)
            file(TO_CMAKE_PATH ${PYTHON_INCLUDE_DIRS} PYTHON_INCLUDE_DIRS)
        endif()

        # PXR_PY_UNDEFINED_DYNAMIC_LOOKUP might be explicitly set when 
        # packaging wheels, or when cross compiling to a Python environment 
        # that is not the current interpreter environment.
        # If it was not explicitly set to ON or OFF, then determine whether 
        # Python was statically linked to its runtime library by fetching the
        # sysconfig variable LDLIBRARY, and set the variable accordingly.
        # If the variable does not exist, PXR_PY_UNDEFINED_DYNAMIC_LOOKUP will
        # default to OFF. On Windows, LDLIBRARY does not exist, as the default
        # will always be OFF.
        if((NOT WIN32) AND (NOT DEFINED PXR_PY_UNDEFINED_DYNAMIC_LOOKUP))
            execute_process(COMMAND ${PYTHON_EXECUTABLE} "-c" "import sysconfig;print(sysconfig.get_config_var('LDLIBRARY'))"
                OUTPUT_STRIP_TRAILING_WHITESPACE
                OUTPUT_VARIABLE PXR_PYTHON_LINKED_LIBRARY
            )
            get_filename_component(PXR_PYTHON_LINKED_LIBRARY_EXT ${PXR_PYTHON_LINKED_LIBRARY} LAST_EXT)
            if(PXR_PYTHON_LINKED_LIBRARY_EXT STREQUAL ".a")
                set(PXR_PY_UNDEFINED_DYNAMIC_LOOKUP ON)
                message(STATUS 
                        "PXR_PY_UNDEFINED_DYNAMIC_LOOKUP wasn't specified, forced ON because Python statically links ${PXR_PYTHON_LINKED_LIBRARY}")
            endif()
        endif()

        # This option indicates that we don't want to explicitly link to the
        # python libraries. See BUILDING.md for details.
        if(PXR_PY_UNDEFINED_DYNAMIC_LOOKUP AND NOT WIN32)
            set(PYTHON_LIBRARIES "")
        else()
            set(PYTHON_LIBRARIES "${package}::Python")
        endif()
    endmacro()

    # USD builds only work with Python3
    setup_python_package(Python3)

    if(WIN32 AND PXR_USE_DEBUG_PYTHON)
        set(Boost_USE_DEBUG_PYTHON ON)
    endif()

    # Manually specify VS2022, 2019, and 2017 as USD's supported compiler versions
    if(WIN32)
        set(Boost_COMPILER "-vc143;-vc142;-vc141")
    endif()

    # As of boost 1.67 the boost_python component name includes the
    # associated Python version (e.g. python27, python36). 
    # XXX: After boost 1.73, boost provided config files should be able to 
    # work without specifying a python version!
    # https://github.com/boostorg/boost_install/blob/master/BoostConfig.cmake

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

    # --Jinja2
    find_package(Jinja2)
else()
    # -- Python
    # A Python interpreter is still required for certain build options.
    if (PXR_BUILD_DOCUMENTATION OR PXR_BUILD_TESTS
        OR PXR_VALIDATE_GENERATED_CODE)

        # We only need to check for Python3 components
        find_package(Python3 COMPONENTS Interpreter)
        set(PYTHON_EXECUTABLE ${Python3_EXECUTABLE})
    endif()
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
if (PXR_BUILD_DOCUMENTATION)
    find_program(DOXYGEN_EXECUTABLE
        NAMES doxygen
    )
    if (EXISTS ${DOXYGEN_EXECUTABLE})                                        
        message(STATUS "Found doxygen: ${DOXYGEN_EXECUTABLE}") 
    else()
        message(FATAL_ERROR 
                "doxygen not found, required for PXR_BUILD_DOCUMENTATION")
    endif()

    find_program(DOT_EXECUTABLE
        NAMES dot
    )
    if (EXISTS ${DOT_EXECUTABLE})
        message(STATUS "Found dot: ${DOT_EXECUTABLE}") 
    else()
        message(FATAL_ERROR
                "dot not found, required for PXR_BUILD_DOCUMENTATION")
    endif()
endif()

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
        set(REQUIRES_Imath TRUE)
        find_package(OpenImageIO REQUIRED)
        add_definitions(-DPXR_OIIO_PLUGIN_ENABLED)
        if (OIIO_idiff_BINARY)
            set(IMAGE_DIFF_TOOL ${OIIO_idiff_BINARY} CACHE STRING "Uses idiff for image diffing")
        endif()
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
    endif()
    # --Metal
    if (PXR_ENABLE_METAL_SUPPORT)
        add_definitions(-DPXR_METAL_SUPPORT_ENABLED)
    endif()
    if (PXR_ENABLE_VULKAN_SUPPORT)
        if (EXISTS $ENV{VULKAN_SDK})
            # Prioritize the VULKAN_SDK includes and packages before any system
            # installed headers. This is to prevent linking against older SDKs
            # that may be installed by the OS.
            # XXX This is fixed in cmake 3.18+
            include_directories(BEFORE SYSTEM $ENV{VULKAN_SDK} $ENV{VULKAN_SDK}/include $ENV{VULKAN_SDK}/lib)
            set(ENV{PATH} "$ENV{VULKAN_SDK}:$ENV{VULKAN_SDK}/include:$ENV{VULKAN_SDK}/lib:$ENV{PATH}")
            find_package(Vulkan REQUIRED)
            list(APPEND VULKAN_LIBS Vulkan::Vulkan)

            # Find the extra vulkan libraries we need
            set(EXTRA_VULKAN_LIBS shaderc_combined)
            foreach(EXTRA_LIBRARY ${EXTRA_VULKAN_LIBS})
                find_library("${EXTRA_LIBRARY}_PATH" NAMES "${EXTRA_LIBRARY}" PATHS $ENV{VULKAN_SDK}/lib)
                list(APPEND VULKAN_LIBS "${${EXTRA_LIBRARY}_PATH}")
            endforeach()

            # Find the OS specific libs we need
            if (APPLE)
                find_library(MVK_LIBRARIES NAMES MoltenVK PATHS $ENV{VULKAN_SDK}/lib)
                list(APPEND VULKAN_LIBS ${MVK_LIBRARIES})
            elseif (UNIX AND NOT APPLE)
                find_package(X11 REQUIRED)
                list(APPEND VULKAN_LIBS ${X11_LIBRARIES})
            elseif (WIN32)
                # No extra libs required
            endif()

            add_definitions(-DPXR_VULKAN_SUPPORT_ENABLED)
        else()
            message(FATAL_ERROR "VULKAN_SDK not valid")
        endif()
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
        set(REQUIRES_Imath TRUE)
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
    set(REQUIRES_Imath TRUE)
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

if (PXR_ENABLE_MATERIALX_SUPPORT)
    find_package(MaterialX REQUIRED)
    add_definitions(-DPXR_MATERIALX_SUPPORT_ENABLED)
endif()

if(PXR_ENABLE_OSL_SUPPORT)
    find_package(OSL REQUIRED)
    set(REQUIRES_Imath TRUE)
    add_definitions(-DPXR_OSL_SUPPORT_ENABLED)
endif()

# ----------------------------------------------

# Try and find Imath or fallback to OpenEXR
# Use ImathConfig.cmake, 
# Refer: https://github.com/AcademySoftwareFoundation/Imath/blob/main/docs/PortingGuide2-3.md#openexrimath-3x-only
if(REQUIRES_Imath)
    find_package(Imath CONFIG)
    if (NOT Imath_FOUND)
        MESSAGE(STATUS "Imath not found. Looking for OpenEXR instead.")
        find_package(OpenEXR REQUIRED)
    endif()
endif()

set(BUILD_SHARED_LIBS "${build_shared_libs}")
