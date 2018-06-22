set(PXR_PREFIX pxr/usdImaging)
set(PXR_PACKAGE usdSkelImaging)

pxr_library(usdSkelImaging
    LIBRARIES
        hd
        pxOsd
        usdImaging
        usdSkel

    PUBLIC_CLASSES
        skeletonAdapter
        utils

    PUBLIC_HEADERS
        api.h

    RESOURCE_FILES
        plugInfo.json
)
