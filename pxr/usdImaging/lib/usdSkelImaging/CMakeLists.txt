set(PXR_PREFIX pxr/usdImaging)
set(PXR_PACKAGE usdSkelImaging)

pxr_library(usdSkelImaging
    LIBRARIES
        hd
        pxOsd
        usdImaging
        usdSkel

    PUBLIC_CLASSES
        skelRootAdapter
        utils

    PUBLIC_HEADERS
        api.h

    RESOURCE_FILES
        plugInfo.json
)
