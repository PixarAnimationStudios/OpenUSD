cmake_minimum_required(VERSION 3.14)

project(usd)

if (NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    if (WIN32)
        message(FATAL_ERROR "Compiler does not support 64-bit builds. "
                "If you are using Visual Studio, make sure you are in the "
                "x64 command prompt and have specified the Win64 cmake "
                "generator (e.g., -G \"Visual Studio 15 2017 Win64\")")
    else()
        message(FATAL_ERROR "Compiler does not support 64-bit builds.")
    endif()
endif()

list(APPEND CMAKE_MODULE_PATH
    ${PROJECT_SOURCE_DIR}/cmake/defaults
    ${PROJECT_SOURCE_DIR}/cmake/modules
    ${PROJECT_SOURCE_DIR}/cmake/macros)

include(Options)
include(ProjectDefaults)
include(Packages)

# This has to be defined after Packages is included, because it relies on the
# discovered path to the python executable.
set(PXR_PYTHON_SHEBANG "${PYTHON_EXECUTABLE}" 
    CACHE 
    STRING
    "Replacement path for Python #! line."
)

# CXXDefaults will set a variety of variables for the project.
# Consume them here. This is an effort to keep the most common
# build files readable.
include(CXXDefaults)
add_definitions(${_PXR_CXX_DEFINITIONS})
set(CMAKE_CXX_FLAGS "${_PXR_CXX_FLAGS} ${CMAKE_CXX_FLAGS}")

include(Public)

pxr_toplevel_prologue()

add_subdirectory(pxr)
add_subdirectory(extras)

if (PXR_BUILD_PRMAN_PLUGIN)
    set(prmanPluginPath "third_party/renderman-${RENDERMAN_VERSION_MAJOR}")

    get_filename_component(prmanPluginAbsPath "${prmanPluginPath}" ABSOLUTE)
    if (EXISTS "${prmanPluginAbsPath}")
        add_subdirectory("${prmanPluginPath}")
    else()
        message(FATAL_ERROR "No RenderMan plugin available for "
                "RenderMan ${RENDERMAN_VERSION_MAJOR}")
    endif()
endif()

if (PXR_BUILD_TESTS)
    pxr_tests_prologue()
endif()

if (PXR_BUILD_DOCUMENTATION)
    pxr_build_documentation()

    # If building python doc strings, run script
    # that uses doxygen build results and installed
    # USD python modules
    if (PXR_BUILD_PYTHON_DOCUMENTATION)
        pxr_build_python_documentation()
    endif()
endif()   

pxr_toplevel_epilogue()