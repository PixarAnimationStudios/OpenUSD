set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE hgiWebGPU)

if (NOT ${PXR_BUILD_GPU_SUPPORT} OR NOT ${PXR_ENABLE_WEBGPU_SUPPORT})
    message(STATUS
            "Skipping ${PXR_PACKAGE} because PXR_BUILD_GPU_SUPPORT or PXR_ENABLE_WEBGPU_SUPPORT is OFF")
    return()
endif()

if(NOT EMSCRIPTEN)
    message(STATUS "DAWN WebGPU support is ON")
    set(PXR_ENABLE_PRECOMPILED_HEADERS OFF)
    set(PXR_ENABLE_PRECOMPILED_HEADERS_PREV ${PXR_ENABLE_PRECOMPILED_HEADERS})
    message("Storing PXR_ENABLE_PRECOMPILED_HEADERS:${PXR_ENABLE_PRECOMPILED_HEADERS_PREV}")
    message(STATUS "Set to compiler flag /std:c++17")

    set(LIB_PREFIX ${CMAKE_STATIC_LIBRARY_PREFIX})
    set(HGI_WEBGPU_LIBRARIES)
    if (WIN32)
        list(APPEND HGI_WEBGPU_LIBRARIES
            dxguid.lib
            ${CMAKE_INSTALL_PREFIX}/lib/absl_strings${CMAKE_STATIC_LIBRARY_SUFFIX}
            ${CMAKE_INSTALL_PREFIX}/lib/absl_strings_internal${CMAKE_STATIC_LIBRARY_SUFFIX}
            ${CMAKE_INSTALL_PREFIX}/lib/absl_str_format_internal${CMAKE_STATIC_LIBRARY_SUFFIX}
            ${CMAKE_INSTALL_PREFIX}/lib/absl_int128${CMAKE_STATIC_LIBRARY_SUFFIX}
            ${CMAKE_INSTALL_PREFIX}/lib/absl_raw_logging_internal${CMAKE_STATIC_LIBRARY_SUFFIX}
            ${CMAKE_INSTALL_PREFIX}/lib/absl_throw_delegate${CMAKE_STATIC_LIBRARY_SUFFIX}
            ${CMAKE_INSTALL_PREFIX}/lib/absl_base${CMAKE_STATIC_LIBRARY_SUFFIX}
        )
    endif()
endif()

pxr_library(hgiWebGPU
    LIBRARIES
        arch
        hgi
        tf
        trace
        hgiGL
        ${HGI_WEBGPU_LIBRARIES}
        ${OPENSUBDIV_LIBRARIES}
        ${Dawn_LIBRARIES}
        glslang::glslang
        glslang::glslang-default-resource-limits
        glslang::MachineIndependent
        glslang::SPIRV
        glslang::GenericCodeGen
        ${SPIRV-Tools-opt_LIBRARIES}
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

    PRIVATE_CLASSES
        debugCodes

    RESOURCE_FILES
        plugInfo.json
)

if (NOT EMSCRIPTEN)
    message("Restoring PXR_ENABLE_PRECOMPILED_HEADERS:${PXR_ENABLE_PRECOMPILED_HEADERS_PREV}")
    set(PXR_ENABLE_PRECOMPILED_HEADERS ${PXR_ENABLE_PRECOMPILED_HEADERS_PREV})
endif()