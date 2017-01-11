set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usdShade)

pxr_shared_library(usdShade
    LIBRARIES
        tf
        vt
        sdf
        usd
        usdGeom

    PUBLIC_CLASSES
        connectableAPI
        debugCodes
        interfaceAttribute
        look
        material
        output
        parameter
        pShader
        pShaderUtils
        shader
        subgraph
        tokens
        utils

    CPPFILES
        moduleDeps.cpp

    PYMODULE_CPPFILES
        module.cpp
        wrapConnectableAPI.cpp
        wrapInterfaceAttribute.cpp
        wrapLook.cpp
        wrapMaterial.cpp
        wrapOutput.cpp
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

