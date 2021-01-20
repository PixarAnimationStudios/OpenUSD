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
        computeCmds
        computePipeline
        graphicsCmds
        graphicsCmdsDesc
        graphicsPipeline
        hgi
        resourceBindings
        sampler
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
