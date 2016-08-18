set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usdShade)

if(WIN32)
    set(MODULE_ENTRYPOINT_CPP DllMain.cpp)
endif()

pxr_shared_library(usdShade
    LIBRARIES
        tf
        vt
        sdf
        usd
        usdGeom

    PUBLIC_CLASSES
        debugCodes
        interfaceAttribute
        look
        material
        parameter
        pShader
        pShaderUtils
        shader
        subgraph
        tokens
        utils
        
    PUBLIC_HEADERS
        api.h

    CPPFILES
        ${MODULE_ENTRYPOINT_CPP}
        moduleDeps.cpp

    PYMODULE_CPPFILES
        module.cpp
        wrapInterfaceAttribute.cpp
        wrapLook.cpp
        wrapMaterial.cpp
        wrapParameter.cpp
        wrapPShader.cpp
        wrapShader.cpp
        wrapSubgraph.cpp
        wrapTokens.cpp

    PYTHON_FILES
        __init__.py

    RESOURCE_FILES
        plugInfo.json
        generatedSchema.usda
)

