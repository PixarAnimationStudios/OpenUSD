set(PXR_PREFIX pxr/hdEmscripten)
set(PXR_PACKAGE hdEmscripten)

if (NOT PXR_ENABLE_JS_SUPPORT)
    return()
endif()

set(BINDINGS_NAME "emHdBindings")

pxr_library(hdEmscripten
    LIBRARIES
      hd
      usd
      usdUtils
      sdf

    PUBLIC_CLASSES
        webRenderDelegate

    PUBLIC_HEADERS
        webRenderDelegate.h
)


target_link_options(hdEmscripten_internal PRIVATE "SHELL: -sEXPORT_NAME=getUsdModule -sMODULARIZE=1 -lembind -sFORCE_FILESYSTEM=1")
target_compile_options(hdEmscripten_internal PRIVATE "SHELL: -lembind")

set(RESOURCE_PARAMETERS "")
function(add_resources target)
    # This local var is needed since list append cannot update the PARENT_SCOPE
    set(LOCAL_VAR ${RESOURCE_PARAMETERS})
    get_property(RESSOURCES TARGET ${target} PROPERTY EMSCRIPTEN_RESOURCES)
    list(APPEND LOCAL_VAR "${RESSOURCES}")
    set(RESOURCE_PARAMETERS "${LOCAL_VAR}" PARENT_SCOPE)
endfunction()

add_resources(usdShade)
add_resources(sdf)
add_resources(hd)
add_resources(usdHydra)
add_resources(usdRender)
add_resources(usdVol)
add_resources(usdImaging)
add_resources(usd)
add_resources(usdLux)
add_resources(ar)
add_resources(usdGeom)
add_resources(ndr)

list(APPEND RESOURCE_PARAMETERS "--preload-file ${PROJECT_BINARY_DIR}/plugins_plugInfo.json@/usd/plugInfo.json")

if(PXR_EMSCRIPTEN_NODE)
    list(TRANSFORM RESOURCE_PARAMETERS REPLACE "-preload" "-embed")
endif()

set(BUILD_FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${BINDINGS_NAME}.js
    ${CMAKE_CURRENT_BINARY_DIR}/${BINDINGS_NAME}.wasm
    ${CMAKE_CURRENT_BINARY_DIR}/${BINDINGS_NAME}.worker.js
    ${CMAKE_CURRENT_SOURCE_DIR}/static/hdEmscripten.html
    ${CMAKE_CURRENT_SOURCE_DIR}/static/simpleShading.usda
    ${CMAKE_CURRENT_SOURCE_DIR}/js/ThreeJsRenderDelegate.js
    ${CMAKE_CURRENT_SOURCE_DIR}/js/OrbitControls.js
    ${CMAKE_CURRENT_SOURCE_DIR}/js/coi-serviceworker.min.js
    ${CMAKE_CURRENT_SOURCE_DIR}/environments/helicopter-landing-pad-vis-4K.hdr
)

if(NOT PXR_EMSCRIPTEN_NODE)
  list(APPEND BUILD_FILES "${CMAKE_CURRENT_BINARY_DIR}/${BINDINGS_NAME}.data")
endif()

pxr_cpp_bin(${BINDINGS_NAME}
  LIBRARIES
      hdEmscripten
      usdImaging
      ${RESOURCE_PARAMETERS}
)

set_target_properties(${BINDINGS_NAME}
    PROPERTIES
        SUFFIX ".js"
)
target_link_options(${BINDINGS_NAME} PRIVATE "SHELL:-sEXPORT_NAME=getUsdModule -sMODULARIZE=1 -lembind -sFORCE_FILESYSTEM=1")
target_compile_options(${BINDINGS_NAME} PRIVATE "SHELL: -lembind")

install(
    FILES
    ${BUILD_FILES}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)