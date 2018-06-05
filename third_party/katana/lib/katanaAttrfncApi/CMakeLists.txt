# This library builds source files from the Katana installation
set(PXR_PACKAGE katanaAttrfncApi)

file(GLOB cppFiles
    ${KATANA_API_SOURCE_DIR}/FnAttribute/client/*.cpp
    ${KATANA_API_SOURCE_DIR}/FnPluginManager/client/*.cpp
    ${KATANA_API_SOURCE_DIR}/FnPluginSystem/client/*.cpp
    ${KATANA_API_SOURCE_DIR}/FnAttributeFunction/plugin/*.cpp
)

pxr_static_library(${PXR_PACKAGE}
    INCLUDE_DIRS
        ${KATANA_API_INCLUDE_DIRS}

    CPPFILES
        ${cppFiles}
)

target_compile_definitions(${PXR_PACKAGE}
    PRIVATE
    
    FNATTRIBUTE_STATIC
    FNATTRIBUTEFUNCTION_STATIC
    FNGEOLIB_STATIC
    FNPLUGINMANAGER_STATIC
    PYSTRING_STATIC
)
