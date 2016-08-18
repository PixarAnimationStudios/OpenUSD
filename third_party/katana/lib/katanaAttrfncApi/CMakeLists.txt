# This library builds source files from the Katana installation
set(PXR_PACKAGE katanaAttrfncApi)

file(GLOB cppFiles
    ${KATANA_API_SOURCE_DIR}/FnAttribute/client/*.cpp
    ${KATANA_API_SOURCE_DIR}/FnPluginManager/client/*.cpp
    ${KATANA_API_SOURCE_DIR}/FnPluginSystem/client/*.cpp
    ${KATANA_API_SOURCE_DIR}/FnAttributeFunction/plugin/*.cpp
)

add_definitions(
    /DFNATTRIBUTE_STATIC
    /DFNATTRIBUTEFUNCTION_STATIC
	/DFNGEOLIB_STATIC
    /DFNPLUGINMANAGER_STATIC
)

pxr_static_library(${PXR_PACKAGE}
    INCLUDE_DIRS
        ${KATANA_API_INCLUDE_DIRS}

    CPPFILES
        ${cppFiles}
)

