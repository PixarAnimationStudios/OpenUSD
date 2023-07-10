set(PXR_PREFIX pxr/usdImaging)
set(PXR_PACKAGE usdviewweb)

set(RESOURCE_PARAMETERS "")
function(add_resources target)
    add_dependencies(${PXR_PACKAGE} ${target})
    target_link_libraries(${PXR_PACKAGE} -Wl,--whole-archive ${target} -Wl,--no-whole-archive)
    # This local var is needed since list append cannot update the PARENT_SCOPE
    set(LOCAL_VAR ${RESOURCE_PARAMETERS})
    get_property(RESSOURCES TARGET ${target} PROPERTY EMSCRIPTEN_RESOURCES)
    list(APPEND LOCAL_VAR "${RESSOURCES}")
    set(RESOURCE_PARAMETERS "${LOCAL_VAR}" PARENT_SCOPE)
endfunction()

pxr_cpp_bin(${PXR_PACKAGE}
    INCLUDE_DIRS
        ${TBB_INCLUDE_DIRS}
)

if (EMSCRIPTEN)
    add_resources(ar)
    add_resources(usdShade)
    add_resources(sdf)
    add_resources(glf)
    add_resources(hd)
    add_resources(hdGp)
    add_resources(hdSt)
    add_resources(hdStorm)
    add_resources(hdx)
    add_resources(hgi)
    add_resources(hgiWebGPU)
    add_resources(usdHydra)
    add_resources(usdShaders)
    add_resources(usdImaging)
    add_resources(usd)
    add_resources(usdGeom)
    add_resources(usdRender)
    add_resources(usdVol)
    add_resources(ndr)

    list(APPEND RESOURCE_PARAMETERS "--preload-file ${PROJECT_BINARY_DIR}/plugins_plugInfo.json@/usd/plugInfo.json")
    list(APPEND RESOURCE_PARAMETERS "--preload-file ${PROJECT_BINARY_DIR}/plugins_plugInfo.json@/plugin/usd/plugInfo.json")

    set(USD_EMSCRIPTEN_LINKER_FLAGS_OPEN "-Wl,--whole-archive")
    set(USD_EMSCRIPTEN_LINKER_FLAGS_CLOSE "-Wl,--no-whole-archive")

    # XXX: We manually set the libraries instead of using the LIBRARIES argument pxr_cpp_bin due to
    # problems with the tbb library.
    target_link_libraries(${PXR_PACKAGE}
            ${RESOURCE_PARAMETERS}
            ${CMAKE_INSTALL_PREFIX}/lib/libshaderc_combined.a
            ${CMAKE_INSTALL_PREFIX}/lib/libtint.a
            ${CMAKE_INSTALL_PREFIX}/lib/libtint_diagnostic_utils.a
            ${TBB_tbb_LIBRARY}
            ${USD_EMSCRIPTEN_LINKER_FLAGS_OPEN}
            cameraUtil
            garch
            geomUtil
            gf
            hdar
            hdsi
            sdrGlslfx
            hf
            hgiInterop
            hio
            js
            kind
            plug
            pxOsd
            pcp
            tf
            trace
            usdImagingGL
            sdr
            vt
            work
            ${USD_EMSCRIPTEN_LINKER_FLAGS_CLOSE}
    )
    set(BUILD_FILES
            ${CMAKE_CURRENT_BINARY_DIR}/${PXR_PACKAGE}.js
            ${CMAKE_CURRENT_BINARY_DIR}/${PXR_PACKAGE}.wasm
            ${CMAKE_CURRENT_BINARY_DIR}/${PXR_PACKAGE}.data
            ${CMAKE_CURRENT_BINARY_DIR}/${PXR_PACKAGE}.worker.js
            ${CMAKE_CURRENT_SOURCE_DIR}/scripts/wasm-server.py
        )

    set(ASYNCIFY_REMOVE "-sASYNCIFY -sASYNCIFY_REMOVE=[\"*glslang*\",\"*tint*\",\"*spv::Builder*\",\"*spvtools*\",\"*UsdGeom*\",\"*WorkDispatcher*\",\"*Usd_CrateFile*\",\"*Sdf*\",\"*boost*\",\"*tbb*\"]")
    set(MODEL "Knife")
    set(MODEL_EXT "usda")
    set(CMAKE_EXE_LINKER_FLAGS_USDVIEWWEB " --tracing --bind ${DEBUG_OPTIONS}  -s DISABLE_EXCEPTION_CATCHING=0 \
         -s ENVIRONMENT=web,worker -s NODERAWFS=0 -s WASM=1 -sALLOW_MEMORY_GROWTH -sUSE_PTHREADS -s MAXIMUM_MEMORY=4GB \
        -s PTHREAD_POOL_SIZE=8 -s NO_EXIT_RUNTIME=1")
    target_link_options(${PXR_PACKAGE} PRIVATE "SHELL:${CMAKE_EXE_LINKER_FLAGS_USDVIEWWEB} ${ASYNCIFY_REMOVE} -sUSE_GLFW=3 -s USE_WEBGL2=1 -sUSE_WEBGPU=1 -O0")
    target_compile_options(${PXR_PACKAGE} PRIVATE "SHELL:${CMAKE_EXE_LINKER_FLAGS_USDVIEWWEB} -sMODULARIZE=0 -O0")
    add_compile_definitions(MODEL_NAME="${MODEL}" MODEL_EXT_NAME="${MODEL_EXT}")
    target_link_options(${PXR_PACKAGE} PRIVATE "SHELL:--preload-file ${CMAKE_CURRENT_SOURCE_DIR}/resources/${MODEL}.${MODEL_EXT}@${MODEL}.${MODEL_EXT} \
        --preload-file ${CMAKE_CURRENT_SOURCE_DIR}/resources/webgpu-env-settings@/webgpu-env-settings \
        -mbulk-memory -matomics --pre-js ${CMAKE_CURRENT_SOURCE_DIR}/resources/preload.js \
        ")

    install(
        FILES
        ${BUILD_FILES}
        DESTINATION ${CMAKE_BINARY_DIR}/../../bin
    )
endif()

target_sources(${PXR_PACKAGE} PRIVATE
        camera.cpp
)