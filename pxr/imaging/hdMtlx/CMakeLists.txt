set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE hdMtlx)

pxr_library(hdMtlx
    LIBRARIES
        gf
        hd
        sdf
        sdr
        tf
        trace
        vt
        ${MATERIALX_LIBRARIES}

    INCLUDE_DIRS
        ${MATERIALX_INCLUDE_DIRS}

    PUBLIC_CLASSES
        hdMtlx

    PUBLIC_HEADERS
        api.h
)
