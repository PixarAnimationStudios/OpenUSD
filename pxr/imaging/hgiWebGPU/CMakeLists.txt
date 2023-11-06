set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE hgiWebGPU)
set(CMAKE_CXX_STANDARD 17)

if (NOT ${PXR_BUILD_GPU_SUPPORT} OR NOT ${PXR_ENABLE_WEBGPU_SUPPORT})
    message(STATUS
            "Skipping ${PXR_PACKAGE} because PXR_BUILD_GPU_SUPPORT or PXR_ENABLE_WEBGPU_SUPPORT is OFF")
    return()
endif()

if(EMSCRIPTEN)
    SET(HGI_WEBGPU_LIBRARIES
        ${CMAKE_INSTALL_PREFIX}/lib/libshaderc_combined.a
        ${CMAKE_INSTALL_PREFIX}/lib/libtint_api.a
        ${CMAKE_INSTALL_PREFIX}/lib/libtint_api_common.a
        ${CMAKE_INSTALL_PREFIX}/lib/libtint_utils_ice.a
        ${CMAKE_INSTALL_PREFIX}/lib/libtint_utils_text.a
        ${CMAKE_INSTALL_PREFIX}/lib/libtint_lang_wgsl_writer_ast_printer.a
        ${CMAKE_INSTALL_PREFIX}/lib/libtint_lang_wgsl_ast.a
        ${CMAKE_INSTALL_PREFIX}/lib/libtint_utils_generator.a
        ${CMAKE_INSTALL_PREFIX}/lib/libtint_utils_rtti.a
        ${CMAKE_INSTALL_PREFIX}/lib/libtint_lang_core.a
        ${CMAKE_INSTALL_PREFIX}/lib/libtint_lang_core_constant.a
        ${CMAKE_INSTALL_PREFIX}/lib/libtint_lang_core_intrinsic.a
        ${CMAKE_INSTALL_PREFIX}/lib/libtint_lang_core_intrinsic_data.a
        ${CMAKE_INSTALL_PREFIX}/lib/libtint_lang_core_type.a
        ${CMAKE_INSTALL_PREFIX}/lib/libtint_utils_id.a
        ${CMAKE_INSTALL_PREFIX}/lib/libtint_utils_strconv.a
        ${CMAKE_INSTALL_PREFIX}/lib/libtint_utils_symbol.a
        ${CMAKE_INSTALL_PREFIX}/lib/libtint_lang_wgsl_ast_transform.a
        ${CMAKE_INSTALL_PREFIX}/lib/libtint_lang_wgsl_resolver.a
        ${CMAKE_INSTALL_PREFIX}/lib/libtint_lang_wgsl_sem.a
        ${CMAKE_INSTALL_PREFIX}/lib/libtint_lang_spirv_reader.a
        ${CMAKE_INSTALL_PREFIX}/lib/libtint_lang_spirv_reader_ast_parser.a
        ${CMAKE_INSTALL_PREFIX}/lib/libtint_utils_debug.a
        ${CMAKE_INSTALL_PREFIX}/lib/libtint_lang_wgsl_writer.a
        ${CMAKE_INSTALL_PREFIX}/lib/libtint_lang_wgsl_program.a
        ${CMAKE_INSTALL_PREFIX}/lib/libtint_utils_diagnostic.a
        )
else()
    add_compile_definitions(_LIBCPP_ENABLE_CXX17_REMOVED_UNARY_BINARY_FUNCTION)
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
        set(STATIC_LIB_SUFFIX ${CMAKE_STATIC_LIBRARY_SUFFIX}) # .lib (static library)
        set(SHARED_LIB_SUFFIX ${CMAKE_STATIC_LIBRARY_SUFFIX}) # .lib (dynamic library)

        # dawn libs need to link for Windows
        list(APPEND HGI_WEBGPU_LIBRARIES
            dxguid.lib
            ${CMAKE_INSTALL_PREFIX}/lib/${LIB_PREFIX}dawn_common${STATIC_LIB_SUFFIX}
            ${CMAKE_INSTALL_PREFIX}/lib/SPIRV-Tools-opt${STATIC_LIB_SUFFIX}
            ${CMAKE_INSTALL_PREFIX}/lib/SPIRV-Tools${STATIC_LIB_SUFFIX}
            ${CMAKE_INSTALL_PREFIX}/lib/absl_strings${STATIC_LIB_SUFFIX}
            ${CMAKE_INSTALL_PREFIX}/lib/absl_strings_internal${STATIC_LIB_SUFFIX}
            ${CMAKE_INSTALL_PREFIX}/lib/absl_str_format_internal${STATIC_LIB_SUFFIX}
            ${CMAKE_INSTALL_PREFIX}/lib/absl_int128${STATIC_LIB_SUFFIX}
            ${CMAKE_INSTALL_PREFIX}/lib/absl_raw_logging_internal${STATIC_LIB_SUFFIX}
            ${CMAKE_INSTALL_PREFIX}/lib/absl_throw_delegate${STATIC_LIB_SUFFIX}
            ${CMAKE_INSTALL_PREFIX}/lib/absl_base${STATIC_LIB_SUFFIX}

            # tint
            ${CMAKE_INSTALL_PREFIX}/lib/${LIB_PREFIX}tint_api_options${STATIC_LIB_SUFFIX}
            ${CMAKE_INSTALL_PREFIX}/lib/${LIB_PREFIX}tint_api_common${STATIC_LIB_SUFFIX}
            ${CMAKE_INSTALL_PREFIX}/lib/${LIB_PREFIX}tint_lang_core${STATIC_LIB_SUFFIX}
            ${CMAKE_INSTALL_PREFIX}/lib/${LIB_PREFIX}tint_lang_core_constant${STATIC_LIB_SUFFIX}
            ${CMAKE_INSTALL_PREFIX}/lib/${LIB_PREFIX}tint_lang_core_intrinsic${STATIC_LIB_SUFFIX}
            ${CMAKE_INSTALL_PREFIX}/lib/${LIB_PREFIX}tint_lang_core_intrinsic_data${STATIC_LIB_SUFFIX}
            ${CMAKE_INSTALL_PREFIX}/lib/${LIB_PREFIX}tint_lang_core_type${STATIC_LIB_SUFFIX}
            ${CMAKE_INSTALL_PREFIX}/lib/${LIB_PREFIX}tint_lang_hlsl_writer${STATIC_LIB_SUFFIX}
            ${CMAKE_INSTALL_PREFIX}/lib/${LIB_PREFIX}tint_lang_hlsl_writer_ast_printer${STATIC_LIB_SUFFIX}
            ${CMAKE_INSTALL_PREFIX}/lib/${LIB_PREFIX}tint_lang_hlsl_writer_ast_raise${STATIC_LIB_SUFFIX}
            ${CMAKE_INSTALL_PREFIX}/lib/${LIB_PREFIX}tint_lang_hlsl_writer_common${STATIC_LIB_SUFFIX}
            ${CMAKE_INSTALL_PREFIX}/lib/${LIB_PREFIX}tint_lang_spirv_reader${STATIC_LIB_SUFFIX}
            ${CMAKE_INSTALL_PREFIX}/lib/${LIB_PREFIX}tint_lang_spirv_reader_ast_lower${STATIC_LIB_SUFFIX}
            ${CMAKE_INSTALL_PREFIX}/lib/${LIB_PREFIX}tint_lang_spirv_reader_ast_parser${STATIC_LIB_SUFFIX}
            ${CMAKE_INSTALL_PREFIX}/lib/${LIB_PREFIX}tint_lang_spirv_reader_common${STATIC_LIB_SUFFIX}
            ${CMAKE_INSTALL_PREFIX}/lib/${LIB_PREFIX}tint_lang_wgsl_ast${STATIC_LIB_SUFFIX}
            ${CMAKE_INSTALL_PREFIX}/lib/${LIB_PREFIX}tint_lang_wgsl_ast_transform${STATIC_LIB_SUFFIX}
            ${CMAKE_INSTALL_PREFIX}/lib/${LIB_PREFIX}tint_lang_wgsl_helpers${STATIC_LIB_SUFFIX}
            ${CMAKE_INSTALL_PREFIX}/lib/${LIB_PREFIX}tint_lang_wgsl_inspector${STATIC_LIB_SUFFIX}
            ${CMAKE_INSTALL_PREFIX}/lib/${LIB_PREFIX}tint_lang_wgsl_program${STATIC_LIB_SUFFIX}
            ${CMAKE_INSTALL_PREFIX}/lib/${LIB_PREFIX}tint_lang_wgsl_reader${STATIC_LIB_SUFFIX}
            ${CMAKE_INSTALL_PREFIX}/lib/${LIB_PREFIX}tint_lang_wgsl_reader_parser${STATIC_LIB_SUFFIX}
            ${CMAKE_INSTALL_PREFIX}/lib/${LIB_PREFIX}tint_lang_wgsl_resolver${STATIC_LIB_SUFFIX}
            ${CMAKE_INSTALL_PREFIX}/lib/${LIB_PREFIX}tint_lang_wgsl_sem${STATIC_LIB_SUFFIX}
            ${CMAKE_INSTALL_PREFIX}/lib/${LIB_PREFIX}tint_lang_wgsl_writer${STATIC_LIB_SUFFIX}
            ${CMAKE_INSTALL_PREFIX}/lib/${LIB_PREFIX}tint_lang_wgsl_writer_ast_printer${STATIC_LIB_SUFFIX}
            ${CMAKE_INSTALL_PREFIX}/lib/${LIB_PREFIX}tint_lang_wgsl_writer_syntax_tree_printer${STATIC_LIB_SUFFIX}
            ${CMAKE_INSTALL_PREFIX}/lib/${LIB_PREFIX}tint_utils_containers${STATIC_LIB_SUFFIX}
            ${CMAKE_INSTALL_PREFIX}/lib/${LIB_PREFIX}tint_utils_debug${STATIC_LIB_SUFFIX}
            ${CMAKE_INSTALL_PREFIX}/lib/${LIB_PREFIX}tint_utils_diagnostic${STATIC_LIB_SUFFIX}
            ${CMAKE_INSTALL_PREFIX}/lib/${LIB_PREFIX}tint_utils_generator${STATIC_LIB_SUFFIX}
            ${CMAKE_INSTALL_PREFIX}/lib/${LIB_PREFIX}tint_utils_ice${STATIC_LIB_SUFFIX}
            ${CMAKE_INSTALL_PREFIX}/lib/${LIB_PREFIX}tint_utils_id${STATIC_LIB_SUFFIX}
            ${CMAKE_INSTALL_PREFIX}/lib/${LIB_PREFIX}tint_utils_macros${STATIC_LIB_SUFFIX}
            ${CMAKE_INSTALL_PREFIX}/lib/${LIB_PREFIX}tint_utils_math${STATIC_LIB_SUFFIX}
            ${CMAKE_INSTALL_PREFIX}/lib/${LIB_PREFIX}tint_utils_memory${STATIC_LIB_SUFFIX}
            ${CMAKE_INSTALL_PREFIX}/lib/${LIB_PREFIX}tint_utils_reflection${STATIC_LIB_SUFFIX}
            ${CMAKE_INSTALL_PREFIX}/lib/${LIB_PREFIX}tint_utils_result${STATIC_LIB_SUFFIX}
            ${CMAKE_INSTALL_PREFIX}/lib/${LIB_PREFIX}tint_utils_rtti${STATIC_LIB_SUFFIX}
            ${CMAKE_INSTALL_PREFIX}/lib/${LIB_PREFIX}tint_utils_strconv${STATIC_LIB_SUFFIX}
            ${CMAKE_INSTALL_PREFIX}/lib/${LIB_PREFIX}tint_utils_symbol${STATIC_LIB_SUFFIX}
            ${CMAKE_INSTALL_PREFIX}/lib/${LIB_PREFIX}tint_utils_text${STATIC_LIB_SUFFIX}
            ${CMAKE_INSTALL_PREFIX}/lib/${LIB_PREFIX}tint_utils_traits${STATIC_LIB_SUFFIX}
        )
    elseif(APPLE)
        set(STATIC_LIB_SUFFIX ${CMAKE_STATIC_LIBRARY_SUFFIX}) # .a (static library)
        set(SHARED_LIB_SUFFIX ${CMAKE_SHARED_LIBRARY_SUFFIX}) # .dylib (dynamic library)
    endif()

    # shared libs for multiple platforms
    list(APPEND HGI_WEBGPU_LIBRARIES
        # dawn
        ${CMAKE_INSTALL_PREFIX}/lib/${LIB_PREFIX}dawn_native${SHARED_LIB_SUFFIX}
        ${CMAKE_INSTALL_PREFIX}/lib/${LIB_PREFIX}dawn_platform${SHARED_LIB_SUFFIX}
        ${CMAKE_INSTALL_PREFIX}/lib/${LIB_PREFIX}dawn_proc${SHARED_LIB_SUFFIX}
        ${CMAKE_INSTALL_PREFIX}/lib/${LIB_PREFIX}dawn_wire${SHARED_LIB_SUFFIX}
        ${CMAKE_INSTALL_PREFIX}/lib/${LIB_PREFIX}webgpu_dawn${SHARED_LIB_SUFFIX}
        ${CMAKE_INSTALL_PREFIX}/lib/${LIB_PREFIX}dawncpp${STATIC_LIB_SUFFIX}

        # tint
        ${CMAKE_INSTALL_PREFIX}/lib/${LIB_PREFIX}tint_api${STATIC_LIB_SUFFIX}

        # shaderc
        ${CMAKE_INSTALL_PREFIX}/lib/${LIB_PREFIX}shaderc_shared${SHARED_LIB_SUFFIX}
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

set(CMAKE_CXX_STANDARD 14)
message(STATUS "Recover compiler flag to /std:c++14")

if (NOT EMSCRIPTEN)
    message("Restoring PXR_ENABLE_PRECOMPILED_HEADERS:${PXR_ENABLE_PRECOMPILED_HEADERS_PREV}")
    set(PXR_ENABLE_PRECOMPILED_HEADERS ${PXR_ENABLE_PRECOMPILED_HEADERS_PREV})
endif()