set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE hgi)

pxr_library(hgi
    LIBRARIES
        gf
        plug
        tf
        hio

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
        unitTestHelper

    PUBLIC_HEADERS
        api.h
        blitCmdsOps.h
        enums.h
        handle.h
)

if (${PXR_HEADLESS_TEST_MODE})
    message(STATUS "Skipping ${PXR_PACKAGE} tests because PXR_HEADLESS_TEST_MODE is ON")
    return()
endif()

if (APPLE)
    message(STATUS "Skipping ${PXR_PACKAGE} tests because they are currently unsupported on macOS")
    return()
endif()

if (WIN32)
    message(STATUS "Skipping ${PXR_PACKAGE} tests because they are currently unsupported on Windows")
    return()
endif()

pxr_build_test(testHgiCommand
    LIBRARIES
        hgi
        garch
        tf
    CPPFILES
        testenv/testHgiCommand.cpp
)
