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

    PUBLIC_HEADERS
        api.h
        vulkan.h
        vk_mem_alloc.h

    PRIVATE_CLASSES
        spirv_reflect

    RESOURCE_FILES
        plugInfo.json
)
