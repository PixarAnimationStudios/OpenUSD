set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE hgiWebGPU)

if (NOT ${PXR_BUILD_GPU_SUPPORT} OR NOT ${PXR_ENABLE_WEBGPU_SUPPORT})
    message(STATUS
            "Skipping ${PXR_PACKAGE} because PXR_BUILD_GPU_SUPPORT or PXR_ENABLE_WEBGPU_SUPPORT is OFF")
    return()
endif()

set(GLSLANG_LIBS
    ${CMAKE_INSTALL_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}glslang${CMAKE_STATIC_LIBRARY_SUFFIX}
    ${CMAKE_INSTALL_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}glslang-default-resource-limits${CMAKE_STATIC_LIBRARY_SUFFIX}
    ${CMAKE_INSTALL_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}MachineIndependent${CMAKE_STATIC_LIBRARY_SUFFIX}
    ${CMAKE_INSTALL_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}SPIRV${CMAKE_STATIC_LIBRARY_SUFFIX}
    ${CMAKE_INSTALL_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}SPIRV-Tools${CMAKE_STATIC_LIBRARY_SUFFIX}
    ${CMAKE_INSTALL_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}SPIRV-Tools-opt${CMAKE_STATIC_LIBRARY_SUFFIX}
    ${CMAKE_INSTALL_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}GenericCodeGen${CMAKE_STATIC_LIBRARY_SUFFIX}
)

set(TINT_COMMON_LIBRARIES ${CMAKE_INSTALL_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}tint_api${CMAKE_STATIC_LIBRARY_SUFFIX})
if(EMSCRIPTEN OR WIN32)
    list(APPEND TINT_COMMON_LIBRARIES
        ${CMAKE_INSTALL_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}tint_api_common${CMAKE_STATIC_LIBRARY_SUFFIX}
        ${CMAKE_INSTALL_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}tint_utils_ice${CMAKE_STATIC_LIBRARY_SUFFIX}
        ${CMAKE_INSTALL_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}tint_utils_text${CMAKE_STATIC_LIBRARY_SUFFIX}
        ${CMAKE_INSTALL_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}tint_lang_wgsl_ast${CMAKE_STATIC_LIBRARY_SUFFIX}
        ${CMAKE_INSTALL_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}tint_utils_generator${CMAKE_STATIC_LIBRARY_SUFFIX}
        ${CMAKE_INSTALL_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}tint_utils_rtti${CMAKE_STATIC_LIBRARY_SUFFIX}
        ${CMAKE_INSTALL_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}tint_lang_core${CMAKE_STATIC_LIBRARY_SUFFIX}
        ${CMAKE_INSTALL_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}tint_lang_core_constant${CMAKE_STATIC_LIBRARY_SUFFIX}
        ${CMAKE_INSTALL_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}tint_lang_core_intrinsic${CMAKE_STATIC_LIBRARY_SUFFIX}
        ${CMAKE_INSTALL_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}tint_lang_core_type${CMAKE_STATIC_LIBRARY_SUFFIX}
        ${CMAKE_INSTALL_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}tint_utils_id${CMAKE_STATIC_LIBRARY_SUFFIX}
        ${CMAKE_INSTALL_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}tint_utils_result${CMAKE_STATIC_LIBRARY_SUFFIX}
        ${CMAKE_INSTALL_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}tint_utils_strconv${CMAKE_STATIC_LIBRARY_SUFFIX}
        ${CMAKE_INSTALL_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}tint_utils_symbol${CMAKE_STATIC_LIBRARY_SUFFIX}
        ${CMAKE_INSTALL_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}tint_lang_core_intrinsic_data${CMAKE_STATIC_LIBRARY_SUFFIX}
        ${CMAKE_INSTALL_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}tint_lang_wgsl${CMAKE_STATIC_LIBRARY_SUFFIX}
        ${CMAKE_INSTALL_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}tint_lang_wgsl_ast_transform${CMAKE_STATIC_LIBRARY_SUFFIX}
        ${CMAKE_INSTALL_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}tint_lang_wgsl_program${CMAKE_STATIC_LIBRARY_SUFFIX}
        ${CMAKE_INSTALL_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}tint_lang_wgsl_reader${CMAKE_STATIC_LIBRARY_SUFFIX}
        ${CMAKE_INSTALL_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}tint_lang_wgsl_reader_parser${CMAKE_STATIC_LIBRARY_SUFFIX}
        ${CMAKE_INSTALL_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}tint_lang_wgsl_resolver${CMAKE_STATIC_LIBRARY_SUFFIX}
        ${CMAKE_INSTALL_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}tint_lang_wgsl_sem${CMAKE_STATIC_LIBRARY_SUFFIX}
        ${CMAKE_INSTALL_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}tint_lang_wgsl_writer_ast_printer${CMAKE_STATIC_LIBRARY_SUFFIX}
        ${CMAKE_INSTALL_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}tint_lang_spirv_reader${CMAKE_STATIC_LIBRARY_SUFFIX}
        ${CMAKE_INSTALL_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}tint_lang_spirv_reader_ast_lower${CMAKE_STATIC_LIBRARY_SUFFIX}
        ${CMAKE_INSTALL_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}tint_lang_spirv_reader_ast_parser${CMAKE_STATIC_LIBRARY_SUFFIX}
        ${CMAKE_INSTALL_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}tint_utils_debug${CMAKE_STATIC_LIBRARY_SUFFIX}
        ${CMAKE_INSTALL_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}tint_lang_wgsl_writer${CMAKE_STATIC_LIBRARY_SUFFIX}
        ${CMAKE_INSTALL_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}tint_utils_diagnostic${CMAKE_STATIC_LIBRARY_SUFFIX}
    )
endif()

if(NOT EMSCRIPTEN)
    if (APPLE)
        add_compile_definitions(_LIBCPP_ENABLE_CXX17_REMOVED_UNARY_BINARY_FUNCTION)
    endif()
    message(STATUS "DAWN WebGPU support is ON")
    set(PXR_ENABLE_PRECOMPILED_HEADERS OFF)
    set(PXR_ENABLE_PRECOMPILED_HEADERS_PREV ${PXR_ENABLE_PRECOMPILED_HEADERS})
    message("Storing PXR_ENABLE_PRECOMPILED_HEADERS:${PXR_ENABLE_PRECOMPILED_HEADERS_PREV}")
    message(STATUS "Set to compiler flag /std:c++17")

    set(LIB_PREFIX ${CMAKE_STATIC_LIBRARY_PREFIX})
    set(HGI_WEBGPU_LIBRARIES)
    if (WIN32)
        # uses Dawn static library for windows with option BUILD_SHARED_LIBS=OFF
        # Note: we got Dawn bulid errors for shared library with option BUILD_SHARED_LIBS=ON
        set(LIB_SUFFIX ${CMAKE_STATIC_LIBRARY_SUFFIX}) # .lib (dynamic library)

        list(APPEND HGI_WEBGPU_LIBRARIES
            dxguid.lib
            ${CMAKE_INSTALL_PREFIX}/lib/${LIB_PREFIX}dawn_common${CMAKE_STATIC_LIBRARY_SUFFIX}
            ${CMAKE_INSTALL_PREFIX}/lib/absl_strings${CMAKE_STATIC_LIBRARY_SUFFIX}
            ${CMAKE_INSTALL_PREFIX}/lib/absl_strings_internal${CMAKE_STATIC_LIBRARY_SUFFIX}
            ${CMAKE_INSTALL_PREFIX}/lib/absl_str_format_internal${CMAKE_STATIC_LIBRARY_SUFFIX}
            ${CMAKE_INSTALL_PREFIX}/lib/absl_int128${CMAKE_STATIC_LIBRARY_SUFFIX}
            ${CMAKE_INSTALL_PREFIX}/lib/absl_raw_logging_internal${CMAKE_STATIC_LIBRARY_SUFFIX}
            ${CMAKE_INSTALL_PREFIX}/lib/absl_throw_delegate${CMAKE_STATIC_LIBRARY_SUFFIX}
            ${CMAKE_INSTALL_PREFIX}/lib/absl_base${CMAKE_STATIC_LIBRARY_SUFFIX}
        )

        list(APPEND TINT_COMMON_LIBRARIES
            ${CMAKE_INSTALL_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}tint_lang_hlsl_writer${CMAKE_STATIC_LIBRARY_SUFFIX}
            ${CMAKE_INSTALL_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}tint_lang_hlsl_writer_ast_printer${CMAKE_STATIC_LIBRARY_SUFFIX}
            ${CMAKE_INSTALL_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}tint_lang_hlsl_writer_ast_raise${CMAKE_STATIC_LIBRARY_SUFFIX}
            ${CMAKE_INSTALL_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}tint_lang_hlsl_writer_common${CMAKE_STATIC_LIBRARY_SUFFIX}
            ${CMAKE_INSTALL_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}tint_lang_wgsl_helpers${CMAKE_STATIC_LIBRARY_SUFFIX}
            ${CMAKE_INSTALL_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}tint_lang_wgsl_inspector${CMAKE_STATIC_LIBRARY_SUFFIX}
        )
    else()
        set(LIB_SUFFIX ${CMAKE_SHARED_LIBRARY_SUFFIX})
    endif()

    # shared libs for multiple platforms
    list(APPEND HGI_WEBGPU_LIBRARIES
        # dawn
        ${CMAKE_INSTALL_PREFIX}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}dawn_native${LIB_SUFFIX}
        ${CMAKE_INSTALL_PREFIX}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}dawn_platform${LIB_SUFFIX}
        ${CMAKE_INSTALL_PREFIX}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}dawn_proc${LIB_SUFFIX}
        ${CMAKE_INSTALL_PREFIX}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}dawn_wire${LIB_SUFFIX}
        ${CMAKE_INSTALL_PREFIX}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}webgpu_dawn${LIB_SUFFIX}
        ${CMAKE_INSTALL_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}dawncpp${CMAKE_STATIC_LIBRARY_SUFFIX}
    )
endif()

pxr_library(hgiWebGPU
    LIBRARIES
        arch
        hgi
        tf
        trace
        hgiGL
        ${HGI_WEBGPU_LIBRARIES}
        ${GLSLANG_LIBS}
        ${TINT_COMMON_LIBRARIES}

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
    find_path(GLSLANG_INCLUDE_DIRS glslang/SPIRV/GlslangToSpv.h PATH_SUFFIXES include)
    # This is required in case we are building with Vulkan support. We want the dawn brought dependencies to be found first.
    target_include_directories(hgiWebGPU BEFORE PRIVATE ${GLSLANG_INCLUDE_DIRS})
    target_link_directories(hgiWebGPU BEFORE PRIVATE ${CMAKE_INSTALL_PREFIX}/lib)
    message("Restoring PXR_ENABLE_PRECOMPILED_HEADERS:${PXR_ENABLE_PRECOMPILED_HEADERS_PREV}")
    set(PXR_ENABLE_PRECOMPILED_HEADERS ${PXR_ENABLE_PRECOMPILED_HEADERS_PREV})
endif()