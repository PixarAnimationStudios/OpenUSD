set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE hgiGL)

if (NOT ${PXR_ENABLE_GL_SUPPORT})
    message(STATUS
        "Skipping ${PXR_PACKAGE} because PXR_ENABLE_GL_SUPPORT is OFF")
    return()
endif()

pxr_library(hgiGL
    LIBRARIES
        arch
        garch
        hgi
        tf
        trace

    PRIVATE_CLASSES
        conversions
        diagnostic
        scopedStateHolder
        shaderGenerator
        shaderSection

    PUBLIC_CLASSES
        blitCmds
        buffer
        capabilities
        computeCmds
        computePipeline
        contextArena
        debugCodes
        device
        garbageCollector
        graphicsCmds
        graphicsPipeline
        hgi
        ops
        resourceBindings
        sampler
        shaderFunction
        shaderProgram
        texture

    PUBLIC_HEADERS
        api.h

    RESOURCE_FILES
        plugInfo.json
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

# This test is not currently supported on static builds as it relies on 
# testHgiCommand, which is not linked in the static hgiGL library. 
if (BUILD_SHARED_LIBS)

pxr_install_test_dir(
    SRC testenv/testHgiGLCommand
    DEST testHgiGLCommand
)
pxr_register_test(testHgiGLCommand
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHgiCommand --write testHgiGLCommand_triangle.png"
    IMAGE_DIFF_COMPARE
        testHgiGLCommand_triangle.png
    FAIL 1
    FAIL_PERCENT 0.001
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
)

endif() # BUILD_SHARED_LIBS
