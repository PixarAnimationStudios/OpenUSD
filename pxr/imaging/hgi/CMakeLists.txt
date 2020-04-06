set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE hgi)

pxr_library(hgi
    LIBRARIES
        gf
        plug
        tf

    PUBLIC_CLASSES
        attachmentDesc
        blitCmds
        buffer
        graphicsCmds
        graphicsCmdsDesc
        hgi
        pipeline
        resourceBindings
        shaderFunction
        shaderProgram
        texture
        tokens
        types
        cmds

    PUBLIC_HEADERS
        api.h
        blitCmdsOps.h
        enums.h
        handle.h
)
