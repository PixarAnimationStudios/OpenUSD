set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usdProc)

pxr_library(usdProc
    LIBRARIES
        tf
        usd
        usdGeom

    PUBLIC_CLASSES
        generativeProcedural
        tokens

    PUBLIC_HEADERS
        api.h

    PYTHON_CPPFILES
        moduleDeps.cpp

    PYMODULE_CPPFILES
        module.cpp
        wrapTokens.cpp
        wrapGenerativeProcedural.cpp

    PYMODULE_FILES
        __init__.py

    RESOURCE_FILES
        plugInfo.json
        generatedSchema.usda
        schema.usda:usdProc/schema.usda

    DOXYGEN_FILES
        overview.dox

)
