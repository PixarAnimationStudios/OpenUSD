set(PXR_PREFIX examples)
set(PXR_PACKAGE httpResolver)

# ---------------------------------------------------------------------------
# HTTP backend selection
# ---------------------------------------------------------------------------
if(APPLE OR IOS)
    set(_defaultBackend "NSURL")
else()
    set(_defaultBackend "CURL")
endif()
set(PXR_HTTP_BACKEND "${_defaultBackend}" CACHE STRING
    "HTTP transport backend: CURL or NSURL (default: NSURL on Apple, CURL elsewhere)")

# ---------------------------------------------------------------------------
# Optional feature flags
# ---------------------------------------------------------------------------
option(PXR_HTTP_USDZ_STREAMING
    "Enable byte-range streaming for USDZ sub-assets" ON)
set(PXR_HTTP_CACHE_MODE "MEMORY" CACHE STRING
    "Asset cache mode: MEMORY (default) or DISK")
set(PXR_HTTP_DISK_CACHE_PATH "" CACHE PATH
    "Root directory for disk-based asset cache (empty = system temp)")
option(PXR_HTTP_SERVER_FILEPATH
    "Allow server to return a local file path instead of file data" OFF)

# ---------------------------------------------------------------------------
# Source selection
# ---------------------------------------------------------------------------
set(_coreSources
    httpResolver.cpp
    httpResolver.h
    packageData.h
)

if(PXR_HTTP_BACKEND STREQUAL "NSURL")
    find_library(FOUNDATION_LIB Foundation REQUIRED)
    set(_backendSources httpResolver_Foundation.mm)
    set(_backendLibs ${FOUNDATION_LIB})
else()
    find_package(CURL REQUIRED)
    set(_backendSources httpResolver_Curl.cpp)
    set(_backendLibs CURL::libcurl)
endif()

# ---------------------------------------------------------------------------
# Build the plugin using the pxr_plugin macro
# ---------------------------------------------------------------------------
pxr_plugin(${PXR_PACKAGE}
    LIBRARIES
        ar
        arch
        tf
        plug
        vt
        ${_backendLibs}

    CPPFILES
        ${_coreSources}
        ${_backendSources}

    RESOURCE_FILES
        plugInfo.json
)

# Ensure .mm files are compiled as Objective-C++ (usually automatic on Apple,
# but explicit in case pxr_plugin's internal rules override the extension).
if(PXR_HTTP_BACKEND STREQUAL "NSURL")
    set_source_files_properties(
        httpResolver_Foundation.mm
        PROPERTIES COMPILE_FLAGS "-x objective-c++"
    )
endif()

# ---------------------------------------------------------------------------
# Generate the feature-flag header consumed by C++ and tests
# ---------------------------------------------------------------------------
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/httpResolverConfig.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/httpResolverConfig.h
)
target_include_directories(${PXR_PACKAGE}
    PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/httpResolverConfig.h
    DESTINATION
        ${CMAKE_INSTALL_PREFIX}/include/pxr/${PXR_PACKAGE}
)

# ---------------------------------------------------------------------------
# Compile-time definitions
# ---------------------------------------------------------------------------
set(_defs "")
if(PXR_HTTP_USDZ_STREAMING)
    list(APPEND _defs PXR_HTTP_USDZ_STREAMING)
endif()
if(PXR_HTTP_CACHE_MODE STREQUAL "DISK")
    list(APPEND _defs PXR_HTTP_CACHE_MODE_DISK)
    if(NOT PXR_HTTP_DISK_CACHE_PATH STREQUAL "")
        list(APPEND _defs "PXR_HTTP_DISK_CACHE_PATH=\"${PXR_HTTP_DISK_CACHE_PATH}\"")
    endif()
endif()
if(PXR_HTTP_SERVER_FILEPATH)
    list(APPEND _defs PXR_HTTP_SERVER_FILEPATH)
endif()
if(_defs)
    target_compile_definitions(${PXR_PACKAGE} PRIVATE ${_defs})
endif()

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------
pxr_test_scripts(
    testenv/testHttpResolver.py
)

pxr_register_test(testHttpResolver
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHttpResolver"
    TESTENV testenv/testHttpResolver
    EXPECTED_RETURN_CODE 0
    ENV
        ${PXR_PLUGINPATH_NAME}=${CMAKE_INSTALL_PREFIX}/share/usd/examples/plugin/${PXR_PACKAGE}/resources
)
