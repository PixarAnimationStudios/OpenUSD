# This library builds source files from the Katana installation
set(PXR_PACKAGE katanaOpApi)

file(GLOB cppFiles
    ${KATANA_API_SOURCE_DIR}/FnAsset/client/*.cpp
    ${KATANA_API_SOURCE_DIR}/FnAsset/plugin/*.cpp
    ${KATANA_API_SOURCE_DIR}/FnAttribute/client/*.cpp
    ${KATANA_API_SOURCE_DIR}/FnGeolib/op/*.cpp
    ${KATANA_API_SOURCE_DIR}/FnGeolib/util/*.cpp
    ${KATANA_API_SOURCE_DIR}/FnGeolibServices/client/*.cpp
    ${KATANA_API_SOURCE_DIR}/FnLogging/client/*.cpp
    ${KATANA_API_SOURCE_DIR}/FnPluginManager/client/*.cpp
    ${KATANA_API_SOURCE_DIR}/FnPluginSystem/client/*.cpp
    ${KATANA_API_SOURCE_DIR}/pystring/.cpp
)

add_definitions(
    /DFNATTRIBUTE_STATIC
    /DFNASSET_STATIC
    /DFNGEOLIB_STATIC
    /DFNGEOLIBSERVICES_STATIC
    /DFNPLUGINMANAGER_STATIC
    /DFNLOGGING_STATIC
)

# https://msdn.microsoft.com/en-us/library/esew7y1w.aspx
add_definitions("/wd4251")

# https://msdn.microsoft.com/en-us/library/3tdb471s.aspx
add_definitions("/wd4275")

pxr_static_library(${PXR_PACKAGE}
    INCLUDE_DIRS
        ${KATANA_API_INCLUDE_DIRS}

    CPPFILES
        ${cppFiles}
)

