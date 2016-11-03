project(usd)

if (WIN32)
    cmake_minimum_required(VERSION 3.1.1)
else()
    cmake_minimum_required(VERSION 2.8.8)
endif()

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/defaults
                      ${CMAKE_SOURCE_DIR}/cmake/modules
                      ${CMAKE_SOURCE_DIR}/cmake/macros)

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
set(CMAKE_CXX_FLAGS ${_PXR_CXX_FLAGS})

if(POLICY CMP0020)
    cmake_policy(SET CMP0020 OLD)
endif()

include(Public)

add_subdirectory(pxr)
add_subdirectory(extras)

function(_addIfExists pkg pkgCmakeLists)
    if (EXISTS ${pkgCmakeLists})
        add_subdirectory(${pkg})
    else()
        message(WARNING "Specified third party package '${pkgName}' was not found. Please check that '${pkgCmakeLists}' exists")
    endif()
endfunction()

function(_enableThirdPartyPlugin identifier)
    set(pkg third_party/${identifier}) 
    set(pkgCmakeLists ${CMAKE_CURRENT_SOURCE_DIR}/${pkg}/CMakeLists.txt)
    _addIfExists(${pkg} ${pkgCmakeLists})
endfunction()

if (${PXR_BUILD_KATANA_PLUGIN})
    if (NOT ${PXR_BUILD_USD_IMAGING})
        message(WARNING "Skipping third_party/katana because PXR_BUILD_USD_IMAGING=OFF")
    else()
        _enableThirdPartyPlugin("katana")
    endif()
endif()

if (${PXR_BUILD_MAYA_PLUGIN})
    if (NOT ${PXR_BUILD_USD_IMAGING})
        message(WARNING "Skipping third_party/maya because PXR_BUILD_USD_IMAGING=OFF")
    else()
        _enableThirdPartyPlugin("maya")
    endif()
endif()
