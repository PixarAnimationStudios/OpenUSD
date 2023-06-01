set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE hdSt)

# XXX We tmp check for PXR_ENABLE_GL_SUPPORT since Storm currently still only
# runs on OpenGL. Once Storm uses only Hgi, remove GL_SUPPORT check.
if (NOT ${PXR_BUILD_GPU_SUPPORT} OR NOT (${PXR_ENABLE_GL_SUPPORT} OR APPLEIOS))
    message(STATUS
        "Skipping ${PXR_PACKAGE} because PXR_BUILD_GPU_SUPPORT is OFF")
    return()
endif()

set(optionalLibs "")
set(optionalIncludeDirs "")
set(optionalPublicClasses "")
set(optionalPrivateClasses "")

if (NOT (APPLEIOS))
    list(APPEND optionalLibs hgiGL)
endif()

if (${PXR_ENABLE_MATERIALX_SUPPORT})
    list(APPEND optionalLibs
        MaterialXGenShader
        MaterialXRender
        MaterialXCore
        MaterialXFormat
        MaterialXGenGlsl
        hdMtlx
    )
    list(APPEND optionalPrivateClasses
         materialXFilter
         materialXShaderGen
    )
endif()
if (PXR_ENABLE_PTEX_SUPPORT)
    list(APPEND optionalLibs ${PTEX_LIBRARY})
    list(APPEND optionalIncludeDirs ${PTEX_INCLUDE_DIR})
    list(APPEND optionalPublicClasses ptexMipmapTextureLoader)
endif()

pxr_library(hdSt
    LIBRARIES
        hio
        garch
        glf
        hd
        hdsi
        hgiInterop
        sdr
        tf
        trace
        ${OPENSUBDIV_LIBRARIES}
        ${optionalLibs}

    INCLUDE_DIRS
        ${OPENSUBDIV_INCLUDE_DIR}
        ${optionalIncludeDirs}

    PUBLIC_CLASSES
        basisCurves
        binding
        bufferArrayRange
        bufferResource
        commandBuffer
        copyComputation
        debugCodes
        dependencyForwardingSceneIndexPlugin
        dependencySceneIndexPlugin
        dispatchBuffer
        domeLightComputations
        drawItem
        drawItemInstance
        drawTarget
        drawTargetRenderPassState
        dynamicUvTextureObject
        extComputation
        extCompGpuComputation
        extCompGpuComputationResource
        extCompGpuPrimvarBufferSource
        fallbackLightingShader
        field
        fieldSubtextureIdentifier
        flatNormals
        geometricShader
        glConversions
        glslfxShader
        glslProgram
        bufferUtils
        hgiConversions
        hioConversions
        imageShaderRenderPass
        implicitSurfaceSceneIndexPlugin
        instancer
        interleavedMemoryManager
        light      
        lightingShader
        material
        materialNetwork
        mesh
        package
        points
        primUtils
        ptexTextureObject
        renderBuffer
        renderDelegate
        renderParam
        renderPass
        renderPassShader
        renderPassState
        resourceRegistry
        samplerObject
        samplerObjectRegistry
        shaderCode
        simpleLightingShader
        smoothNormals
        stagingBuffer
        subtextureIdentifier
        textureBinder
        textureCpuData
        textureHandle
        textureHandleRegistry
        textureIdentifier
        textureObject
        textureObjectRegistry
        textureUtils
        tokens
        udimTextureObject
        unitTestGLDrawing
        vboMemoryManager
        vboSimpleMemoryManager
        volume
        ${optionalPublicClasses}

    PUBLIC_HEADERS
        api.h
        dynamicUvTextureImplementation.h

    PRIVATE_CLASSES
        assetUvTextureCpuData
        basisCurvesComputations
        basisCurvesShaderKey
        basisCurvesTopology
        codeGen
        cullingShaderKey
        drawBatch
        drawItemsCache
        extCompComputeShader
        fieldTextureCpuData
        imageShaderShaderKey
        indirectDrawBatch
        materialNetworkShader
        materialBindingResolvingSceneIndexPlugin
        materialParam
        meshShaderKey
        meshTopology 
        nodeIdentifierResolvingSceneIndex
        nodeIdentifierResolvingSceneIndexPlugin
        pipelineDrawBatch
        pointsShaderKey
        quadrangulate
        resourceBinder
        resourceLayout
        shaderKey
        subdivision
        terminalsResolvingSceneIndexPlugin
        triangulate
        unitTestHelper
        volumeShader
        volumeShaderKey
        ${optionalPrivateClasses}

    RESOURCE_FILES
        plugInfo.json
        shaders/basisCurves.glslfx
        shaders/compute.glslfx
        shaders/domeLight.glslfx
        shaders/edgeId.glslfx
        shaders/fallbackLighting.glslfx
        shaders/fallbackLightingShader.glslfx
        shaders/fallbackMaterialNetwork.glslfx
        shaders/fallbackVolume.glslfx
        shaders/frustumCull.glslfx
        shaders/imageShader.glslfx
        shaders/instancing.glslfx
        shaders/mesh.glslfx
        shaders/meshFaceCull.glslfx
        shaders/meshNormal.glslfx
        shaders/meshWire.glslfx
        shaders/points.glslfx
        shaders/pointId.glslfx
        shaders/ptexTexture.glslfx
        shaders/renderPass.glslfx
        shaders/renderPassShader.glslfx
        shaders/simpleLightingShader.glslfx
        shaders/terminals.glslfx
        shaders/visibility.glslfx
        shaders/volume.glslfx
        textures/fallbackBlackDomeLight.png

    DOXYGEN_FILES
        overview.dox
)

if (X11_FOUND OR APPLE)
pxr_build_test(testHdStBasicDrawing
    LIBRARIES
        hdSt
        hd
        garch
        glf
    CPPFILES
        testenv/testHdStBasicDrawing.cpp
)
endif()
