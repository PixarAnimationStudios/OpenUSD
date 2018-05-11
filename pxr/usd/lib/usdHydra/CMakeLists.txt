set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usdHydra)

pxr_library(usdHydra

    LIBRARIES
        tf
        usd
        usdShade

    PUBLIC_CLASSES
        tokens

    PUBLIC_HEADERS
        api.h

    PYTHON_CPPFILES
        moduleDeps.cpp

    PYMODULE_CPPFILES
        module.cpp
        wrapTokens.cpp

    PYMODULE_FILES
        __init__.py

    RESOURCE_FILES
        plugInfo.json
)

