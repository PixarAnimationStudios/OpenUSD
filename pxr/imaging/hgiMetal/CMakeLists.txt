set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE hgiMetal)

if (NOT ${PXR_BUILD_GPU_SUPPORT} OR NOT ${PXR_ENABLE_METAL_SUPPORT})
    message(STATUS
        "Skipping ${PXR_PACKAGE} because PXR_BUILD_GPU_SUPPORT or PXR_ENABLE_METAL_SUPPORT is OFF")
    return()
endif()

FIND_LIBRARY(METAL_LIBRARY Metal)
FIND_LIBRARY(APPUIKIT_LIBRARY AppKit)

pxr_library(hgiMetal
    LIBRARIES
        arch
        hgi
        tf
        trace
        ${METAL_LIBRARY}
        ${APPUIKIT_LIBRARY}

    PUBLIC_HEADERS
        api.h
        capabilities.h
        computeCmds.h
        computePipeline.h
        diagnostic.h
        hgi.h
        texture.h
        blitCmds.h
        buffer.h
        graphicsCmds.h
        graphicsPipeline.h
        resourceBindings.h
        sampler.h
        shaderFunction.h
        shaderProgram.h

    PRIVATE_HEADERS
        conversions.h
        shaderGenerator.h
        shaderSection.h

    CPPFILES
        blitCmds.mm
        buffer.mm
        capabilities.mm
        computePipeline.mm
        computeCmds.mm
        conversions.mm
        diagnostic.mm
        graphicsCmds.mm
        graphicsPipeline.mm
        hgi.mm
        resourceBindings.mm
        sampler.mm
        shaderFunction.mm
        shaderGenerator.mm
        shaderProgram.mm
        shaderSection.mm
        texture.mm

    RESOURCE_FILES
        plugInfo.json
)
