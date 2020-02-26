set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE hgi)

pxr_library(hgi
    LIBRARIES
        gf
        plug
        tf

    PUBLIC_CLASSES
        blitEncoder
        buffer
        graphicsEncoder
        graphicsEncoderDesc
        hgi
        immediateCommandBuffer
        pipeline
        resourceBindings
        shaderFunction
        shaderProgram
        texture
        tokens

    PUBLIC_HEADERS
        api.h
        blitEncoderOps.h
        enums.h
        handle.h
        types.h
)
