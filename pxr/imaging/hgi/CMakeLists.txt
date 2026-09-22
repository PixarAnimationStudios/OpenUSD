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
        externalBuffer
        externalBufferArena
        graphicsCmds
        graphicsCmdsDesc
        graphicsPipeline
        hgi
        indirectCommandEncoder
        resourceBindings
        sampler
        semaphore
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
        version.h
)

# Pure CPU: no device, no display, no GL context. Registered ABOVE the
# headless / APPLE / WIN32 early returns below on purpose -- those exist for
# tests that drive a GPU, and an early return there would silently mean this
# test does not exist on most of the platforms that need it.
pxr_build_test(testHgiExternalBufferArena
    LIBRARIES
        hgi
        tf
    CPPFILES
        testenv/testHgiExternalBufferArena.cpp
)
# On Windows the spawned test exe needs the USD DLLs and the Python runtime
# (python313.dll, which sits next to the interpreter rather than in bin) on
# PATH. hgi pulls in tf, tf links Python, so without this the loader fails
# before main() runs and ctest reports 0xC0000135 / 3221225781 --
# STATUS_DLL_NOT_FOUND -- which looks like a test failure but is not one.
# PRE_PATH is prepended to PATH by the test wrapper. Empty when Python support
# is off, in which case tf needs no runtime and the entry drops out.
get_filename_component(_hgi_pyRuntimeDir "${PYTHON_EXECUTABLE}" DIRECTORY)
pxr_register_test(testHgiExternalBufferArena
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHgiExternalBufferArena"
    EXPECTED_RETURN_CODE 0
    PRE_PATH
        ${CMAKE_INSTALL_PREFIX}/lib
        ${CMAKE_INSTALL_PREFIX}/bin
        ${_hgi_pyRuntimeDir}
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

if (${PXR_BUILD_GPU_SUPPORT})
    pxr_build_test(testHgiCommand
        LIBRARIES
            hgi
            garch
            tf
        CPPFILES
            testenv/testHgiCommand.cpp
    )
endif()