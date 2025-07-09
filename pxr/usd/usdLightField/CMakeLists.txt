set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usdLightField)

pxr_library(usdLightField
    INCLUDE_SCHEMA_FILES

    LIBRARIES
        plug
        tf
        sdf
        vt
        gf
        usd
        usdGeom

    PUBLIC_HEADERS
        api.h
    
    PYMODULE_FILES
        __init__.py
)
