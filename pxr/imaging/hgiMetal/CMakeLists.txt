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
        diagnostic.h
        hgi.h
        texture.h
        blitCmds.h
        buffer.h
        graphicsCmds.h
        pipeline.h
        resourceBindings.h
        shaderFunction.h
        shaderProgram.h

    PRIVATE_HEADERS
        conversions.h

    CPPFILES
        blitCmds.mm
        buffer.mm
        capabilities.mm
        conversions.mm
        diagnostic.mm
        graphicsCmds.mm
        hgi.mm
        pipeline.mm
        resourceBindings.mm
        shaderFunction.mm
        shaderProgram.mm
        texture.mm

    RESOURCE_FILES
        plugInfo.json
)
