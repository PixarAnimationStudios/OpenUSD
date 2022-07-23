set(PXR_PREFIX pxr/usdImaging)
set(PXR_PACKAGE usdProcImaging)

pxr_library(usdProcImaging
    LIBRARIES
        usdImaging
        usdProc

    PUBLIC_CLASSES
        generativeProceduralAdapter

    PUBLIC_HEADERS
        api.h

    RESOURCE_FILES
        plugInfo.json
)
