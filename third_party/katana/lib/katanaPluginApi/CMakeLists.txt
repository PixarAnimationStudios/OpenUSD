# This library builds source files from the Katana installation
set(PXR_PACKAGE katanaPluginApi)

# Disable all warnings for this library, otherwise strict builds will fail
# due to warnings from this code, which comes from the Katana installation and
# which we have no control over.
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR
    "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    add_definitions("-Wno-error")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    add_definitions("/w")
endif()

file(GLOB cppFiles
    ${KATANA_API_SOURCE_DIR}/FnAsset/client/*.cpp
    ${KATANA_API_SOURCE_DIR}/FnAsset/plugin/*.cpp
    ${KATANA_API_SOURCE_DIR}/FnAttribute/client/*.cpp
    ${KATANA_API_SOURCE_DIR}/FnConfig/*.cpp
    ${KATANA_API_SOURCE_DIR}/FnDefaultAttributeProducer/plugin/*.cpp
    ${KATANA_API_SOURCE_DIR}/FnGeolibServices/client/*.cpp
    ${KATANA_API_SOURCE_DIR}/FnGeolib/op/*.cpp
    ${KATANA_API_SOURCE_DIR}/FnGeolib/util/*.cpp
    ${KATANA_API_SOURCE_DIR}/FnLogging/client/*.cpp
    ${KATANA_API_SOURCE_DIR}/FnPluginManager/client/*.cpp
    ${KATANA_API_SOURCE_DIR}/FnPluginSystem/*.cpp
    ${KATANA_API_SOURCE_DIR}/FnRender/plugin/*.cpp
    ${KATANA_API_SOURCE_DIR}/FnRenderOutputLocation/client/*.cpp
    ${KATANA_API_SOURCE_DIR}/FnRenderOutputLocation/plugin/*.cpp
    ${KATANA_API_SOURCE_DIR}/FnRenderOutputUtils/client/*.cpp
    ${KATANA_API_SOURCE_DIR}/FnScenegraphGenerator/plugin/*.cpp
    ${KATANA_API_SOURCE_DIR}/FnScenegraphIterator/client/*.cpp
    ${KATANA_API_SOURCE_DIR}/FnViewerModifier/plugin/*.cpp
    ${KATANA_API_SOURCE_DIR}/pystring/*.cpp
)

pxr_static_library(${PXR_PACKAGE}
    INCLUDE_DIRS
        ${KATANA_API_INCLUDE_DIRS}

    CPPFILES
        ${cppFiles}
)

