set(PXR_PREFIX pxr/usdImaging)
set(PXR_PACKAGE usdNprImaging)

pxr_library(usdNprImaging
    LIBRARIES
        usd
        usdImaging
        usdNpr

    PUBLIC_CLASSES
        utils
        halfEdge
        dualMesh
        contourAdapter
        tokens

    PUBLIC_HEADERS
        api.h

    RESOURCE_FILES
        plugInfo.json
)
