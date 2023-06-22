set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE hgiDX)

if (NOT ${PXR_BUILD_GPU_SUPPORT} OR NOT ${PXR_ENABLE_DIRECTX_SUPPORT})
    message(STATUS
        "Skipping ${PXR_PACKAGE} because PXR_BUILD_GPU_SUPPORT or PXR_ENABLE_DIRECTX_SUPPORT is OFF")
    return()
endif()

pxr_library(hgiDX
    LIBRARIES
        arch
        hd
        hgi
        d3d12
        dxgi
        dxguid
        d3dcompiler
        tf
        trace


    PUBLIC_CLASSES
        blitCmds
        buffer
        capabilities
        computeCmds
        computePipeline
        conversions
        device
        graphicsCmds
        graphicsPipeline
        hgi
        indirectCommandEncoder
        memoryHelper
        presentation
        resourceBindings
        sampler
        shaderFunction
        shaderGenerator
        shaderProgram
        shaderSection
        texture
        textureConverter

    PUBLIC_HEADERS
        api.h
        shaderInfo.h

    PRIVATE_CLASSES

    RESOURCE_FILES
        plugInfo.json
)

add_executable(shadersTranslatorUtility 
   ShadersTranslatorUtility/shadersAnalysis.cpp 
   ShadersTranslatorUtility/shadersCodeTranslation.cpp
   ShadersTranslatorUtility/shadersDefines.cpp
)

install(TARGETS shadersTranslatorUtility
   RUNTIME DESTINATION "bin"
)

install(
   DIRECTORY ShadersTranslatorUtility/shadersReferences/
   DESTINATION bin/shadersReferences
)

install(CODE "execute_process(COMMAND ${CMAKE_INSTALL_PREFIX}/bin/shadersTranslatorUtility.exe -installDir ${CMAKE_INSTALL_PREFIX} -refDir ${CMAKE_INSTALL_PREFIX}/bin/shadersReferences)")


