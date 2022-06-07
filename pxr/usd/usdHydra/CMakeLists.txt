set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usdHydra)

pxr_library(usdHydra

    LIBRARIES
        tf
        usd
        usdShade

    PUBLIC_CLASSES
        tokens
        discoveryPlugin
        generativeProceduralAPI

    PUBLIC_HEADERS
        api.h

    PYTHON_CPPFILES
        moduleDeps.cpp

    PYMODULE_CPPFILES
        module.cpp
        wrapTokens.cpp
        wrapGenerativeProceduralAPI.cpp

    PYMODULE_FILES
        __init__.py

    RESOURCE_FILES
        plugInfo.json
        generatedSchema.usda
        schema.usda:usdHydra/schema.usda
        shaders/shaderDefs.usda
        shaders/empty.glslfx

    DOXYGEN_FILES
        overview.dox
)

