set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE hdSt)

if (OPENSUBDIV_HAS_GLSL_TRANSFORM_FEEDBACK)
    add_definitions(-DOPENSUBDIV_HAS_GLSL_TRANSFORM_FEEDBACK)
endif()
if (OPENSUBDIV_HAS_GLSL_COMPUTE)
    add_definitions(-DOPENSUBDIV_HAS_GLSL_COMPUTE)
endif()

pxr_library(hdSt
    LIBRARIES
        cameraUtil
        garch
        glf
        hd
        tf
        tracelite
        ${GLEW_LIBRARY}
        ${OPENSUBDIV_LIBRARIES}

    INCLUDE_DIRS
        ${GLEW_INCLUDE_DIR}
        ${OPENSUBDIV_INCLUDE_DIR}

    PUBLIC_CLASSES
        basisCurves
        bufferArrayRangeGL
        bufferResourceGL
        camera
        commandBuffer
        computeShader
        copyComputation
        dispatchBuffer
        drawItem
        drawItemInstance
        drawTarget
        drawTargetAttachmentDesc
        drawTargetAttachmentDescArray
        drawTargetRenderPassState
        extCompGpuComputation
        extCompGpuComputationBufferSource
        extCompGpuComputationResource
        fallbackLightingShader
        geometricShader
        glConversions
        glslfxShader
        glslProgram
        glUtils
        instancer
        interleavedMemoryManager
        light      
        lightingShader
        material
        mesh
        mixinShaderCode
        package
        persistentBuffer
        points
        renderContextCaps
        renderDelegate
        renderPass
        renderPassShader
        renderPassState
        resourceBinder
        resourceGL
        resourceRegistry
        shaderCode
        shaderKey
        smoothNormals
        surfaceShader
        texture
        textureResource
        tokens
        unitTestGLDrawing
        vboMemoryManager
        vboSimpleMemoryManager

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
        immediateDrawBatch
        indirectDrawBatch
        meshShaderKey
        meshTopology 
        pointsShaderKey
        quadrangulate
        subdivision
        subdivision3
        triangulate
        unitTestDelegate
        unitTestHelper

    RESOURCE_FILES
        plugInfo.json
        shaders/basisCurves.glslfx
        shaders/compute.glslfx
        shaders/edgeId.glslfx
        shaders/fallbackLighting.glslfx
        shaders/fallbackLightingShader.glslfx
        shaders/fallbackSurface.glslfx
        shaders/frustumCull.glslfx
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
