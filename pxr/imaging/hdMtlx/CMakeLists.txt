set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE hdMtlx)

add_definitions(-DPXR_MATERIALX_STDLIB_DIR="${MATERIALX_STDLIB_DIR}")

pxr_library(hdMtlx
    LIBRARIES
        gf
        hd
        sdf
        sdr
        tf
        trace
        vt
        MaterialXCore
        MaterialXFormat

    PUBLIC_CLASSES
        hdMtlx

    PUBLIC_HEADERS
        api.h
)
