set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usdHydra)

pxr_library(usdHydra

    INCLUDE_SCHEMA_FILES

    LIBRARIES
        tf
        usd
        usdShade

    PUBLIC_CLASSES
        discoveryPlugin

    PUBLIC_HEADERS
        api.h

    PYMODULE_FILES
        __init__.py

    RESOURCE_FILES
        shaders/shaderDefs.usda
        shaders/empty.glslfx

    DOXYGEN_FILES
        overview.dox
)

