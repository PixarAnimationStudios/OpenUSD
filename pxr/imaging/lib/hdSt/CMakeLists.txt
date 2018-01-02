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
        hd
        tf
        tracelite

    INCLUDE_DIRS
        ${OPENSUBDIV_INCLUDE_DIR}

    PUBLIC_CLASSES
        basisCurves
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
        glslfxShader
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
        renderDelegate
        renderPass
        renderPassShader
        renderPassState
        resourceRegistry
        smoothNormals
        surfaceShader
        texture
        textureResource
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
        shaders/fallbackLighting.glslfx
        shaders/fallbackLightingShader.glslfx
        shaders/fallbackSurface.glslfx
        shaders/lighting.glslfx
        shaders/lightingIntegrationShader.glslfx
        shaders/renderPass.glslfx
        shaders/renderPassShader.glslfx
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
