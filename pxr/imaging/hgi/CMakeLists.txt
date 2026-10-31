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
        version.h
)

if (${PXR_HEADLESS_TEST_MODE})
    message(STATUS "Skipping ${PXR_PACKAGE} tests because PXR_HEADLESS_TEST_MODE is ON")
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

    pxr_build_test(testHgiMipmap
        LIBRARIES
            hgi
            hio
            garch
            tf
        CPPFILES
            testenv/testHgiMipmap.cpp
    )

    pxr_build_test(testHgiCopy
        LIBRARIES
            hgi
            garch
            tf
        CPPFILES
            testenv/testHgiCopy.cpp
    )
endif()

# Registers the backend-agnostic hgi tests for a specific Hgi backend.
# backendName is used to name the test (ex.: Vulkan) and the remaining arguments
# are ENV entries (ex.: HGI_ENABLE_VULKAN=1).
function(pxr_register_hgi_backend_tests backendName)
    if (${PXR_BUILD_GPU_SUPPORT})
        pxr_register_test(testHgi${backendName}Mipmap
            COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHgiMipmap"
            TESTENV ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/testenv/testHgiMipmap
            IMAGE_DIFF_COMPARE
                mipmapLevel0.png
                mipmapLevel1.png
                mipmapLevel2.png
                mipmapLevel3.png
                mipmapLevel4.png
                mipmapLevel5.png
                mipmapLevel6.png
                mipmapLevel7.png
                mipmapLevel8.png
            FAIL 0.01
            FAIL_PERCENT 0.001
            EXPECTED_RETURN_CODE 0
            ENV
                ${ARGN}
        )
        pxr_register_test(testHgi${backendName}MipmapNpt
            COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHgiMipmap --input mipmapInput_npt.png --write mipmapLevelNpt"
            TESTENV ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/testenv/testHgiMipmap
            IMAGE_DIFF_COMPARE
                mipmapLevelNpt0.png
                mipmapLevelNpt1.png
                mipmapLevelNpt2.png
                mipmapLevelNpt3.png
                mipmapLevelNpt4.png
                mipmapLevelNpt5.png
                # Only levels reduced from even dimensions are compared.
                # From 6 and onward it's odd dimensions the filtering is
                # implementation-defined and differs between backends.
            FAIL 0.01
            FAIL_PERCENT 0.001
            EXPECTED_RETURN_CODE 0
            ENV
                ${ARGN}
        )

        pxr_register_test(testHgi${backendName}Copy
            COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHgiCopy"
            EXPECTED_RETURN_CODE 0
            ENV
                ${ARGN}
        )
    endif()
endfunction()
