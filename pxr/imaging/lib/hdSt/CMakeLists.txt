set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE hdSt)

if (NOT ${PXR_ENABLE_GL_SUPPORT})
    message(STATUS
        "Skipping ${PXR_PACKAGE} because PXR_ENABLE_GL_SUPPORT is OFF")
    return()
endif()

if (OPENSUBDIV_HAS_GLSL_TRANSFORM_FEEDBACK)
    add_definitions(-DOPENSUBDIV_HAS_GLSL_TRANSFORM_FEEDBACK)
endif()
if (OPENSUBDIV_HAS_GLSL_COMPUTE)
    add_definitions(-DOPENSUBDIV_HAS_GLSL_COMPUTE)
endif()

pxr_library(hdSt
    LIBRARIES
        hio
        garch
        glf
        hd
        hgiGL
        sdr
        tf
        trace
        ${GLEW_LIBRARY}
        ${OPENSUBDIV_LIBRARIES}

    INCLUDE_DIRS
        ${GLEW_INCLUDE_DIR}
        ${OPENSUBDIV_INCLUDE_DIR}

    PUBLIC_CLASSES
        basisCurves
        bufferArrayRangeGL
        bufferResourceGL
        commandBuffer
        computeShader
        copyComputation
        debugCodes
        dispatchBuffer
        domeLightComputations
        drawItem
        drawItemInstance
        drawTarget
        drawTargetAttachmentDesc
        drawTargetAttachmentDescArray
        drawTargetRenderPassState
        extComputation
        extCompGpuComputation
        extCompGpuComputationBufferSource
        extCompGpuComputationResource
        extCompGpuPrimvarBufferSource
        fallbackLightingShader
        field
        fieldResource
        flatNormals
        geometricShader
        glConversions
        glslfxShader
        glslProgram
        glUtils
        hgiConversions
        imageShaderRenderPass
        instancer
        interleavedMemoryManager
        light      
        lightingShader
        material
        mesh
        mixinShader
        package
        persistentBuffer
        points
        renderBuffer
        renderDelegate
        renderPass
        renderPassShader
        renderPassState
        resourceBinder
        resourceGL
        resourceRegistry
        rprimUtils
        shaderCode
        shaderKey
        smoothNormals
        surfaceShader
        texture
        textureResource
        textureResourceHandle
        tokens
        unitTestGLDrawing
        vboMemoryManager
        vboSimpleMemoryManager
        volume

    PUBLIC_HEADERS
        api.h

    PRIVATE_CLASSES
        basisCurvesComputations
        basisCurvesShaderKey
        basisCurvesTopology
        codeGen
        cullingShaderKey
        drawBatch
        drawTargetTextureResource
        imageShaderShaderKey
        immediateDrawBatch
        indirectDrawBatch
        materialBufferSourceAndTextureHelper
        materialNetwork
        meshShaderKey
        meshTopology 
        pointsShaderKey
        quadrangulate
        subdivision
        subdivision3
        triangulate
        unitTestDelegate
        unitTestHelper
        volumeShader
        volumeShaderKey

    RESOURCE_FILES
        plugInfo.json
        shaders/basisCurves.glslfx
        shaders/compute.glslfx
        shaders/domeLight.glslfx
        shaders/edgeId.glslfx
        shaders/fallbackLighting.glslfx
        shaders/fallbackLightingShader.glslfx
        shaders/fallbackSurface.glslfx
        shaders/fallbackVolume.glslfx
        shaders/frustumCull.glslfx
        shaders/imageShader.glslfx
        shaders/instancing.glslfx
        shaders/lighting.glslfx
        shaders/lightingIntegrationShader.glslfx
        shaders/mesh.glslfx
        shaders/meshNormal.glslfx
        shaders/meshWire.glslfx
        shaders/points.glslfx
        shaders/pointId.glslfx
        shaders/ptexTexture.glslfx
        shaders/renderPass.glslfx
        shaders/renderPassShader.glslfx
        shaders/terminals.glslfx
        shaders/visibility.glslfx
        shaders/volume.glslfx
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
