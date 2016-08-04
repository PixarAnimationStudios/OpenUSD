set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE hd)

set(optionalPublicClasses "")
if (QT4_FOUND)
    list(APPEND optionalPrivateClasses unitTestHelper)
endif()

if (OPENSUBDIV_HAS_GLSL_TRANSFORM_FEEDBACK)
    _add_define(OPENSUBDIV_HAS_GLSL_TRANSFORM_FEEDBACK)
endif()
if (OPENSUBDIV_HAS_GLSL_COMPUTE)
    _add_define(OPENSUBDIV_HAS_GLSL_COMPUTE)
endif()

pxr_shared_library(hd
    LIBRARIES
        plug
        tf
        tracelite
        vt
        work
        sdf
        glf
        pxOsd
        ${OPENSUBDIV_LIBRARIES}
        ${GLEW_LIBRARY}
        ${TBB_LIBRARY}

    INCLUDE_DIRS
        ${OPENSUBDIV_INCLUDE_DIR}
        ${GLEW_INCLUDE_DIR}
        ${TBB_INCLUDE_DIRS}
        ${OPENSUBDIV_INCLUDE_DIR}

    PUBLIC_CLASSES
        basisCurves
        basisCurvesTopology
        binding
        bufferArray
        bufferArrayRange
        bufferArrayRegistry
        bufferResource
        bufferSource
        bufferSpec
        camera
        changeTracker
        codeGen
        commandBuffer
        computation
        conversions
        copyComputation
        debugCodes
        drawTarget
        drawTargetAttachmentDesc
        drawTargetAttachmentDescArray
        drawTargetRenderPass
        drawTargetRenderPassState
        dirtyList
        dispatchBuffer
        drawingCoord
        drawBatch
        drawItem
        drawItemInstance
        drawItemState
        engine
        glslProgram
        glslfxShader
        glUtils
        immediateDrawBatch
        indirectDrawBatch
        instancer
        instanceRegistry
        interleavedMemoryManager
        light
        lightingShader
        mesh
        meshTopology
        package
        patchIndex
        perfLog
        persistentBuffer
        points
        renderContextCaps
        renderIndex
        renderPass
        renderPassShader
        renderPassState
        repr
        resource
        resourceBinder
        resourceRegistry
        rprim
        rprimCollection
        rprimSharedData
        sceneDelegate
        shader
        shaderKey
        shaderParam
        simpleLightingShader
        surfaceShader
        strategyBase
        task
        texture
        textureResource
        tokens
        topology
        triangulate
        vboMemoryManager
        vboSimpleMemoryManager
        vtBufferSource

    PUBLIC_HEADERS
        enums.h
        version.h
        api.h

    PRIVATE_CLASSES
        basisCurvesComputations
        basisCurvesShaderKey
        cullingShaderKey
        defaultLightingShader
        geometricShader
        meshShaderKey
        pointsShaderKey
        quadrangulate
        smoothNormals
        subdivision
        subdivision3
        unitTestDelegate
        vertexAdjacency
        ${optionalPrivateClasses}

    PRIVATE_HEADERS
        unitTestGLDrawing.h

    RESOURCE_FILES
        plugInfo.json
        shaders/basisCurves.glslfx
        shaders/compute.glslfx
        shaders/defaultLightingShader.glslfx
        shaders/fallbackSurface.glslfx
        shaders/frustumCull.glslfx
        shaders/instancing.glslfx
        shaders/mesh.glslfx
        shaders/meshNormal.glslfx
        shaders/meshWire.glslfx
        shaders/points.glslfx
        shaders/ptexTexture.glslfx
        shaders/renderPass.glslfx
        shaders/renderPassShader.glslfx
        shaders/simpleLighting.glslfx
        shaders/simpleLightingShader.glslfx
)

if (QT4_FOUND)
    pxr_build_test(testHdBasicDrawing
        LIBRARIES
            hd
            glf
            glfq
            Qt4::QtCore
            Qt4::QtOpenGL
        CPPFILES
            unitTestGLDrawing.cpp
            testenv/testHdBasicDrawing.cpp
    )
endif()
