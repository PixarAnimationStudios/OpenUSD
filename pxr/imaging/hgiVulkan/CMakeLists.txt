set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE hgiVulkan)

if (NOT ${PXR_BUILD_GPU_SUPPORT} OR NOT ${PXR_ENABLE_VULKAN_SUPPORT})
    message(STATUS
        "Skipping ${PXR_PACKAGE} because PXR_BUILD_GPU_SUPPORT or PXR_ENABLE_VULKAN_SUPPORT is OFF")
    return()
endif()

pxr_library(hgiVulkan
    LIBRARIES
        arch
        hgi
        tf
        trace
        ${VULKAN_LIBS}

    PUBLIC_CLASSES
        blitCmds
        buffer
        capabilities
        commandBuffer
        commandQueue
        computeCmds
        computePipeline
        conversions
        device
        diagnostic
        garbageCollector
        graphicsCmds
        graphicsPipeline
        hgi
        instance
        pipelineCache
        resourceBindings
        sampler
        shaderCompiler
        shaderFunction
        shaderGenerator
        shaderProgram
        shaderSection
        texture
        vk_mem_alloc

    PUBLIC_HEADERS
        api.h
        vulkan.h

    PRIVATE_CLASSES
        spirv_reflect

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

pxr_build_test(testHgiVulkan
    LIBRARIES
        hgiVulkan
        hgi
        hio
        tf
    CPPFILES
        testenv/testHgiVulkan.cpp
)
pxr_install_test_dir(
    SRC testenv/testHgiVulkan
    DEST testHgiVulkan
)
pxr_register_test(testHgiVulkan
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHgiVulkan"
    IMAGE_DIFF_COMPARE
        graphicsCmdsClear.png
        srgba.png
        copyTextureToBuffer.png
        copyBufferToTexture.png
    FAIL 1
    FAIL_PERCENT 0.001
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
)

# This test is not currently supported on static builds as it relies on 
# testHgiCommand, which is not linked in the static hgiVulkan library. 
if (BUILD_SHARED_LIBS)

pxr_install_test_dir(
    SRC testenv/testHgiVulkanCommand
    DEST testHgiVulkanCommand
)
pxr_register_test(testHgiVulkanCommand
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHgiCommand --write testHgiVulkanCommand_triangle.png"
    IMAGE_DIFF_COMPARE
        testHgiVulkanCommand_triangle.png
    FAIL 1
    FAIL_PERCENT 0.001
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    ENV
        HGI_ENABLE_VULKAN=1
        HGIVULKAN_DEBUG=1
        HGIVULKAN_DEBUG_VERBOSE=1
)

endif() # BUILD_SHARED_LIBS
