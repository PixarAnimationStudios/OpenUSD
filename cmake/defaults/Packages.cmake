#
# Copyright 2016 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
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

if(PXR_ENABLE_OPENVDB_SUPPORT)
    # Find Boost package before getting any boost specific components as we need to
    # disable boost-provided cmake config, based on the boost version found.
    find_package(Boost REQUIRED)
    # If a user explicitly sets Boost_NO_BOOST_CMAKE to On, following will
    # disable the use of boost provided cmake config.
    option(Boost_NO_BOOST_CMAKE "Disable boost-provided cmake config" OFF)
    if (Boost_NO_BOOST_CMAKE)
      message(STATUS "Disabling boost-provided cmake config")
    endif()
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

        # This option indicates that we don't want libraries to explicitly link
        # to the Python libraries. However, executables must link to the Python
        # libraries to avoid missing symbol errors. See BUILDING.md for details.
        if(PXR_PY_UNDEFINED_DYNAMIC_LOOKUP AND NOT WIN32)
            set(PYTHON_LIBRARIES "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${package}::Python>")
        else()
            set(PYTHON_LIBRARIES "${package}::Python")
        endif()
    endmacro()

    # USD builds only work with Python3
    setup_python_package(Python3)

    # Compute the default Python bindings install directory from the Python
    # interpreter if the user has not provided an explicit value. Note that
    # we intentionally do not use Python3_SITEARCH since that may be an
    # absolute path pointing outside of our install prefix. We should allow
    # such paths only if they are given by the user.
    if(NOT PXR_PYTHON_INSTALL_DIR)
        execute_process(
            COMMAND "${PYTHON_EXECUTABLE}"
                "${PROJECT_SOURCE_DIR}/cmake/macros/getPythonInstallDir.py"
            OUTPUT_STRIP_TRAILING_WHITESPACE
            OUTPUT_VARIABLE _pxr_default_python_install_dir
            RESULT_VARIABLE _pxr_sysconfig_rc
        )
        if(_pxr_sysconfig_rc EQUAL 0 AND _pxr_default_python_install_dir)
            set(PXR_PYTHON_INSTALL_DIR "${_pxr_default_python_install_dir}"
                CACHE STRING
                "Directory for installing Python bindings." FORCE)
        else()
            set(PXR_PYTHON_INSTALL_DIR "lib/python"
                CACHE STRING
                "Directory for installing Python bindings." FORCE)
            message(WARNING
                "Could not detect Python site-packages path,"
                "defaulting PXR_PYTHON_INSTALL_DIR to lib/python")
        endif()
        unset(_pxr_default_python_install_dir)
        unset(_pxr_sysconfig_rc)
    endif()

    message(STATUS "Installing Python bindings to ${PXR_PYTHON_INSTALL_DIR}")

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

# Convert paths to CMake path format on Windows to avoid string parsing
# issues when we pass PYTHON_EXECUTABLE or PYTHON_INCLUDE_DIRS to
# pxr_library or other functions.
if(WIN32)
    if(PYTHON_EXECUTABLE)
        file(TO_CMAKE_PATH ${PYTHON_EXECUTABLE} PYTHON_EXECUTABLE)
    endif()

    if(PYTHON_INCLUDE_DIRS)
        file(TO_CMAKE_PATH ${PYTHON_INCLUDE_DIRS} PYTHON_INCLUDE_DIRS)
    endif()
endif()

# --TBB
if (DEFINED PXR_FIND_TBB_IN_CONFIG)
    if (PXR_FIND_TBB_IN_CONFIG)
        find_package(TBB CONFIG REQUIRED COMPONENTS tbb)
    else()
        find_package(TBB REQUIRED COMPONENTS tbb)
    endif()
else()
    # Set PXR_FIND_TBB_IN_CONFIG appropriately so that downstream
    # pxrConfig knows how TBB was found and appropriately encodes the 
    # dependency.
    find_package(TBB CONFIG COMPONENTS tbb)
    if (TBB_FOUND)
        set(PXR_FIND_TBB_IN_CONFIG ON)
    else()
        find_package(TBB REQUIRED COMPONENTS tbb)
        set(PXR_FIND_TBB_IN_CONFIG OFF)
    endif()
endif()

# --math
if(WIN32)
    # Math functions are linked automatically by including math.h on Windows.
    set(M_LIB "")
else()
    set(M_LIB m)
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

    if (PXR_BUILD_HTML_DOCUMENTATION)
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
    if (PXR_ENABLE_GL_SUPPORT AND NOT PXR_APPLE_EMBEDDED)
        # Prefer legacy GL library over GLVND libraries if both
        # are installed.
        if (POLICY CMP0072)
            cmake_policy(SET CMP0072 OLD)
        endif()
        find_package(OpenGL REQUIRED)
        add_definitions(-DPXR_GL_SUPPORT_ENABLED)
    endif()
    # --Metal
    if (PXR_ENABLE_METAL_SUPPORT)
        add_definitions(-DPXR_METAL_SUPPORT_ENABLED)
    endif()
    if (PXR_ENABLE_VULKAN_SUPPORT)
        message(STATUS "Enabling experimental feature Vulkan support")
        if (EXISTS $ENV{VULKAN_SDK})
            find_package(Vulkan REQUIRED COMPONENTS shaderc_combined)
            list(APPEND VULKAN_LIBS Vulkan::Vulkan Vulkan::shaderc_combined)

            # Find the OS specific libs we need
            if (UNIX AND NOT APPLE)
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
    set(OPENSUBDIV_USE_GPU ${PXR_BUILD_GPU_SUPPORT})
    find_package(OpenSubdiv 3 CONFIG)
    if(OpenSubdiv_DIR)
        # Found in CONFIG mode.
        # First check the shared, then the static library, just like find_library() in FindOpenSubdiv.cmake.
        foreach(postfix "" "_static")
            if(NOT TARGET OpenSubdiv::osdCPU${postfix})
                continue()
            endif()
            set(OPENSUBDIV_LIBRARIES OpenSubdiv::osdCPU${postfix})
            if(OPENSUBDIV_USE_GPU)
                list(APPEND OPENSUBDIV_LIBRARIES OpenSubdiv::osdGPU${postfix})
            endif()
            break()
        endforeach()
    endif()
    if(OPENSUBDIV_LIBRARIES)
        list(GET OPENSUBDIV_LIBRARIES 0 OPENSUBDIV_OSDCPU_LIBRARY)
        set(PXR_FIND_OPENSUBDIV_IN_CONFIG ON)
    else()
        # Try again with the find-module.
        find_package(OpenSubdiv 3 REQUIRED)
        set(PXR_FIND_OPENSUBDIV_IN_CONFIG OFF)
    endif()
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
        add_definitions(-DPXR_X11_SUPPORT_ENABLED)
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
