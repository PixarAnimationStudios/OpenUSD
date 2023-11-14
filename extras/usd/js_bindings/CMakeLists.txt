set(PXR_PREFIX jsBindings)
set(PXR_PACKAGE jsBindings)

if (NOT PXR_ENABLE_JS_SUPPORT)
  return()
endif()

set(BINDINGS_NAME "jsBindings")
set(BINDING_DEPENDENCIES "")
list(APPEND BINDING_DEPENDENCIES hd usd hgi hgiWebGPU usdLux usdGeom usdUtils sdf tf usdShaders)

set(RESOURCE_PARAMETERS "")
function(add_resources target)
    # This local var is needed since list append cannot update the PARENT_SCOPE
    set(LOCAL_VAR ${RESOURCE_PARAMETERS})
    get_property(RESSOURCES TARGET ${target} PROPERTY EMSCRIPTEN_RESOURCES)
    list(APPEND LOCAL_VAR "${RESSOURCES}")
    set(RESOURCE_PARAMETERS "${LOCAL_VAR}" PARENT_SCOPE)
endfunction()

# Gather resources (schemas) to make them available to attach them to the bundle
add_resources(usdShade)
add_resources(sdf)
add_resources(hd)
add_resources(hgi)
add_resources(hgiWebGPU)
add_resources(usdHydra)
add_resources(usdShaders)
add_resources(usdImaging)
add_resources(usd)
add_resources(ar)
add_resources(usdLux)
add_resources(usdGeom)
add_resources(ndr)

list(APPEND RESOURCE_PARAMETERS "--preload-file ${PROJECT_BINARY_DIR}/plugins_plugInfo.json@/usd/plugInfo.json")

if(PXR_EMSCRIPTEN_NODE)
  list(TRANSFORM RESOURCE_PARAMETERS REPLACE "-preload" "-embed")
endif()

pxr_cpp_bin(${BINDINGS_NAME}
    LIBRARIES
      ${BINDING_DEPENDENCIES}
      ${RESOURCE_PARAMETERS}
)

target_compile_options(${BINDINGS_NAME} PRIVATE "SHELL:-lembind")
target_link_options(${BINDINGS_NAME} PRIVATE "SHELL:-sUSE_WEBGPU=1 -sEXPORT_NAME=getUsdModule -sMODULARIZE=1  -lembind --pre-js ${CMAKE_SOURCE_DIR}/js/prejs.js")
set_target_properties(${BINDINGS_NAME}
    PROPERTIES
        SUFFIX ".js"
)

set(BUILD_FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${BINDINGS_NAME}.js
    ${CMAKE_CURRENT_BINARY_DIR}/${BINDINGS_NAME}.wasm
    ${CMAKE_CURRENT_BINARY_DIR}/${BINDINGS_NAME}.worker.js
    ${CMAKE_SOURCE_DIR}/js/test.html
)

if(NOT PXR_EMSCRIPTEN_NODE)
    list(APPEND BUILD_FILES "${CMAKE_CURRENT_BINARY_DIR}/${BINDINGS_NAME}.data")
endif()

install(
    FILES
      ${BUILD_FILES}
    DESTINATION ${CMAKE_BINARY_DIR}/../../bin
)
# Install files for npm package

install(
    FILES
      ${BUILD_FILES}
    DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/../../../js/bindings/${CMAKE_BUILD_TYPE}
)
#[[
# Create ES6 module - TBD
install(
    CODE
    "execute_process(
        COMMAND
        ${CMAKE_COMMAND}
        -DSOURCE_DIR=${CMAKE_SOURCE_DIR}
        -P
        ${CMAKE_CURRENT_LIST_DIR}/create_es6_module.cmake
    )"
)

install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/usd.js 
    DESTINATION ${CMAKE_BINARY_DIR}/../../bin
)
]]
