set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usdLightField)

pxr_library(usdLightField
    INCLUDE_SCHEMA_FILES

    LIBRARIES
        tf
        sdf
        vt
        gf
        pcp
        usd
        usdGeom
        arch

    CPPFILES
        # List non-schema cpp files

    PUBLIC_HEADERS
        api.h

    PYMODULE_FILES
        __init__.py

    DOXYGEN_FILES
        overview.dox
)
