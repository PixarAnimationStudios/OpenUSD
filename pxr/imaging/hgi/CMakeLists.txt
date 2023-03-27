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
        capabilities
        cmds
        computeCmds
        computeCmdsDesc
        computePipeline
        debugCodes
        graphicsCmds
        graphicsCmdsDesc
        graphicsPipeline
        hgi
        indirectCommandEncoder
        resourceBindings
        sampler
        shaderFunction
        shaderFunctionDesc
        shaderGenerator
        shaderProgram
        shaderSection
        texture
        tokens
        types

    PUBLIC_HEADERS
        api.h
        blitCmdsOps.h
        enums.h
        handle.h
)
