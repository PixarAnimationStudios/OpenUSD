set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE hgiWebGPU)

if (NOT ${PXR_BUILD_GPU_SUPPORT} OR NOT ${PXR_ENABLE_WEBGPU_SUPPORT})
    message(STATUS
            "Skipping ${PXR_PACKAGE} because PXR_BUILD_GPU_SUPPORT or PXR_ENABLE_WEBGPU_SUPPORT is OFF")
    return()
endif()

if(NOT EMSCRIPTEN)
    message(STATUS "DAWN WebGPU support is ON")
    # This prevents an issued from compiling in windows
    set(PXR_ENABLE_PRECOMPILED_HEADERS_PREV ${PXR_ENABLE_PRECOMPILED_HEADERS})
    set(PXR_ENABLE_PRECOMPILED_HEADERS OFF)
    message("Storing PXR_ENABLE_PRECOMPILED_HEADERS:${PXR_ENABLE_PRECOMPILED_HEADERS_PREV}")
    set(HGI_WEBGPU_NATIVE_LIBRARIES dawn::webgpu_dawn)
    if (APPLE)
        list(APPEND HGI_WEBGPU_NATIVE_LIBRARIES
            "-framework IOKit"
            "-framework IOSurface"
            "-framework Metal"
            "-framework QuartzCore"
        )
    elseif (WIN32)
        list(APPEND HGI_WEBGPU_NATIVE_LIBRARIES
            dxguid.lib
        )
    endif()
endif()

pxr_library(hgiWebGPU
    LIBRARIES
        arch
        hgi
        tf
        ts
        trace
        ${HGI_WEBGPU_NATIVE_LIBRARIES}
        ${OPENSUBDIV_LIBRARIES}
        glslang::glslang
        glslang::glslang-default-resource-limits
        glslang::MachineIndependent
        glslang::SPIRV
        glslang::GenericCodeGen
        SPIRV-Tools-opt
        SPIRV-Headers::SPIRV-Headers
        ${Tint_LIBRARIES}

    PUBLIC_CLASSES
        blitCmds
        buffer
        capabilities
        computeCmds
        computePipeline
        conversions
        depthResolver
        diagnostic
        graphicsCmds
        graphicsPipeline
        hgi
        mipmapGenerator
        pipelineBindGroups
        resourceBindings
        sampler
        shaderCompiler
        shaderFunction
        shaderGenerator
        shaderProgram
        shaderSection
        stepFunctions
        texture

    PUBLIC_HEADERS
        api.h

    PRIVATE_HEADERS
        baseGraphicsCmds.h

    PRIVATE_CLASSES
        debugCodes
        spirvTransforms

    RESOURCE_FILES
        plugInfo.json
)

# Dawn and Tint/emdawnwebgpu headers require C++20 (std::span, concepts, etc.)
if (EMSCRIPTEN)
    set(_hgiWebGPUTarget hgiWebGPU_internal)
else()
    set(_hgiWebGPUTarget hgiWebGPU)
endif()
set_target_properties(${_hgiWebGPUTarget} PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

if (NOT EMSCRIPTEN)
    message("Restoring PXR_ENABLE_PRECOMPILED_HEADERS:${PXR_ENABLE_PRECOMPILED_HEADERS_PREV}")
    set(PXR_ENABLE_PRECOMPILED_HEADERS ${PXR_ENABLE_PRECOMPILED_HEADERS_PREV})
endif()
