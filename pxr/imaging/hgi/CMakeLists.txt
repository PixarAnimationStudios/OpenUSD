set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE hgi)

pxr_library(hgi
    LIBRARIES
        gf
        tf

    PUBLIC_CLASSES
        blitEncoder
        buffer
        graphicsEncoder
        graphicsEncoderDesc
        hgi
        immediateCommandBuffer
        texture

    PUBLIC_HEADERS
        api.h
        blitEncoderOps.h
        enums.h
        types.h
)
