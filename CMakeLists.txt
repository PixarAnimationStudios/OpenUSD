project(usd)

if (WIN32)
    cmake_minimum_required(VERSION 3.1.1)
else()
    cmake_minimum_required(VERSION 2.8.8)
endif()

if (NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    if (WIN32)
        message(FATAL_ERROR "Compiler does not support 64-bit builds. "
                "If you are using Visual Studio, make sure you are in the "
                "x64 command prompt and have specified the Win64 cmake "
                "generator (e.g., -G \"Visual Studio 14 2015 Win64\")")
    else()
        message(FATAL_ERROR "Compiler does not support 64-bit builds.")
    endif()
endif()

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/defaults
                      ${CMAKE_SOURCE_DIR}/cmake/modules
                      ${CMAKE_SOURCE_DIR}/cmake/macros)

include(Options)

# Versions prior to 3.1.0 on Unix platforms built archive libraries
# incorrectly.  Require at least that version if necessary.
if((NOT BUILD_SHARED_LIBS OR PXR_BUILD_MONOLITHIC) AND NOT WIN32)
    cmake_minimum_required(VERSION 3.1.0)
endif()

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

if (${PXR_BUILD_KATANA_PLUGIN})
    add_subdirectory(third_party/katana)
endif()

if (${PXR_BUILD_MAYA_PLUGIN})
    add_subdirectory(third_party/maya)
endif()

if (${PXR_BUILD_HOUDINI_PLUGIN})
    add_subdirectory(third_party/houdini)
endif()

if (${PXR_BUILD_DOCUMENTATION})
    # Ensure we have doxygen+graphviz available
    find_program(DOXYGEN_EXECUTABLE
        NAMES doxygen
    )
    if (NOT EXISTS ${DOXYGEN_EXECUTABLE})                                        
        message(WARNING "Cannot run doc generation, doxygen not found.")
        return()
    endif()

    find_program(DOT_EXECUTABLE
        NAMES dot
    )
    if (NOT EXISTS ${DOT_EXECUTABLE})
        message(WARNING "Cannot run doc generation, dot not found.")
        return()
    endif()

    _pxrDoxyConfig_subst()
    
    set(DOCS_DIR "${CMAKE_BINARY_DIR}")
    set(GEN_SCRIPT "${PROJECT_SOURCE_DIR}/cmake/macros/generateDocs.py")
    set(PXR_SOURCE_DIR "${CMAKE_SOURCE_DIR}/pxr")
    set(THIRD_PARTY_SOURCE_DIR "${CMAKE_SOURCE_DIR}/third_party")

    add_custom_target(
        documentation
        ALL
        COMMAND ${PYTHON_EXECUTABLE} ${GEN_SCRIPT} ${PXR_SOURCE_DIR} ${THIRD_PARTY_SOURCE_DIR} ${CMAKE_BINARY_DIR} ${DOCS_DIR} ${DOXYGEN_EXECUTABLE} ${DOT_EXECUTABLE}
        DEPENDS ${CMAKE_BINARY_DIR}/include/pxr/pxr.h
    )

    set(BUILT_HTML_DOCS "${DOCS_DIR}/docs/doxy_html")
    set(INST_DOCS_ROOT  "${CMAKE_INSTALL_PREFIX}/docs")
    install(
        DIRECTORY ${BUILT_HTML_DOCS}
        DESTINATION ${INST_DOCS_ROOT}
    )

    set(BUILT_XML_DOCS "${DOCS_DIR}/docs/doxy_xml")
    install(
        DIRECTORY ${BUILT_XML_DOCS}
        DESTINATION ${INST_DOCS_ROOT}
    )
endif()   

pxr_toplevel_epilogue()
