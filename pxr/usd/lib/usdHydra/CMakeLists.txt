set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usdHydra)

pxr_shared_library(usdHydra

    LIBRARIES
        tf
        usd
        usdShade

    PUBLIC_CLASSES
        lookAPI
        primvar
        shader
        texture
        uvTexture
        ptexTexture
        tokens

    CPPFILES
        moduleDeps.cpp

    PYMODULE_CPPFILES
        module.cpp
        wrapLookAPI.cpp
        wrapPrimvar.cpp
        wrapShader.cpp
        wrapTexture.cpp
        wrapUvTexture.cpp
        wrapPtexTexture.cpp
        wrapTokens.cpp

    PYTHON_FILES
        __init__.py

    RESOURCE_FILES
        plugInfo.json
)

