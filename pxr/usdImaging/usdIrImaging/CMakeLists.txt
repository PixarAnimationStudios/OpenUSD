set(PXR_PREFIX pxr/usdImaging)
set(PXR_PACKAGE usdIrImaging)

if(NOT PXR_BUILD_EXEC)
    return()
endif()

pxr_library(usdIrImaging
    LIBRARIES
        execIr
        gf
        hd
        tf
        usd
        usdGeom
        usdImaging

    PUBLIC_CLASSES
        jointScopeAdapter
        tokens

    PUBLIC_HEADERS
        api.h

    RESOURCE_FILES
        plugInfo.json
)
