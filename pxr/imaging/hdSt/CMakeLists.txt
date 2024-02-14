set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE hdSt)

# XXX We tmp check for PXR_ENABLE_GL_SUPPORT since Storm currently still only
# runs on OpenGL. Once Storm uses only Hgi, remove GL_SUPPORT check.
if (NOT ${PXR_BUILD_GPU_SUPPORT} OR NOT ${PXR_ENABLE_GL_SUPPORT})
    message(STATUS
        "Skipping ${PXR_PACKAGE} because PXR_BUILD_GPU_SUPPORT is OFF")
    return()
endif()

set(optionalLibs "")
set(optionalIncludeDirs "")
set(optionalPublicClasses "")
set(optionalPrivateClasses "")
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
    if (MaterialX_VERSION VERSION_GREATER_EQUAL "1.38.7")
        list(APPEND optionalLibs
            MaterialXGenMsl
        )
    endif()
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
        hgiGL
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
        bufferArrayRegistry
        commandBuffer
        computation
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
        extCompCpuComputation
        extCompGpuComputation
        extCompGpuComputationResource
        extCompGpuPrimvarBufferSource
        extCompPrimvarBufferSource
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
        nurbsApproximatingSceneIndexPlugin
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
        strategyBase
        subtextureIdentifier
        tetMeshConversionSceneIndexPlugin
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
        extCompComputedInputSource
        extCompInputSource
        extCompSceneInputSource
        fieldTextureCpuData
        imageShaderShaderKey
        indirectDrawBatch
        materialNetworkShader
        materialBindingResolvingSceneIndexPlugin
        materialPrimvarTransferSceneIndexPlugin
        materialParam
        meshShaderKey
        meshTopology 
        nodeIdentifierResolvingSceneIndex
        nodeIdentifierResolvingSceneIndexPlugin
        pipelineDrawBatch
        pointsShaderKey
        quadrangulate
        resourceBinder
        shaderKey
        subdivision
        triangulate
        unitTestHelper
        vertexAdjacency
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
        shaders/invalidMaterialNetwork.glslfx
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
        shaders/surfaceHelpers.glslfx
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

if (X11_FOUND)
pxr_build_test(testHdStBarAllocationLimit
    LIBRARIES
        hdSt
        hd
        gf
        tf
    CPPFILES
        testenv/testHdStBarAllocationLimit.cpp
)
pxr_build_test(testHdStBufferAggregation
    LIBRARIES
        hdSt
        hd
        hgi
        garch
        glf
        vt
        tf
        gf
    CPPFILES
        testenv/testHdStBufferAggregation.cpp
)
pxr_build_test(testHdStBufferArray
    LIBRARIES
        hdSt
        hd
        glf
        tf
        gf
    CPPFILES
        testenv/testHdStBufferArray.cpp
)
pxr_build_test(testHdStBufferSource
    LIBRARIES
        hd
        vt
        tf
        gf
    CPPFILES
        testenv/testHdStBufferSource.cpp
)
pxr_build_test(testHdStBufferSourceDependency
    LIBRARIES
        hdSt
        hd
        glf
        tf
    CPPFILES
        testenv/testHdStBufferSourceDependency.cpp
)
pxr_build_test(testHdStCodeGen
    LIBRARIES
        hdSt
        hd
        hio
        glf
        tf
    CPPFILES
        testenv/testHdStCodeGen.cpp
)
pxr_build_test(testHdStCurveDrawing
    LIBRARIES
        hdSt
        hd
        tf
        gf
    CPPFILES
        testenv/testHdStCurveDrawing.cpp
)
pxr_build_test(testHdStCurvePrimvarInterpolation
    LIBRARIES
        hdSt
        hd
        tf
        gf
    CPPFILES
        testenv/testHdStCurvePrimvarInterpolation.cpp
)
pxr_build_test(testHdStCurvesPrimvars
    LIBRARIES
        hdSt
        hd
        glf
        sdf
        tf
        gf
    CPPFILES
        testenv/testHdStCurvesPrimvars.cpp
)
pxr_build_test(testHdStDisplayStyle
    LIBRARIES
        hdSt
        hd
        tf
        gf
    CPPFILES
        testenv/testHdStDisplayStyle.cpp
)
pxr_build_test(testHdStDrawBatching
    LIBRARIES
        hdSt
        hd
        hio
        glf
        tf
        gf
    CPPFILES
        testenv/testHdStDrawBatching.cpp
)
pxr_build_test(testHdStDrawItemIntegrity
    LIBRARIES
        hdSt
        hd
        tf
        gf
    CPPFILES
        testenv/testHdStDrawItemIntegrity.cpp
)
pxr_build_test(testHdStDrawItemsCache
    LIBRARIES
        hdSt
        hd
        tf
        gf
    CPPFILES
        testenv/testHdStDrawItemsCache.cpp
)
pxr_build_test(testHdStDynamicUvTexture
    LIBRARIES
        hdSt
        tf
    CPPFILES
        testenv/testHdStDynamicUvTexture.cpp
)
pxr_build_test(testHdStFaceCulling
    LIBRARIES
        hdSt
        hd
        tf
        gf
    CPPFILES
        testenv/testHdStFaceCulling.cpp
)
pxr_build_test(testHdStFrustumCulling
    LIBRARIES
        hdSt
        tf
        gf
    CPPFILES
        testenv/testHdStFrustumCulling.cpp
)
pxr_build_test(testHdStGL
    LIBRARIES
        garch
        glf
        tf
    CPPFILES
        testenv/testHdStGL.cpp
)
pxr_build_test(testHdStGLSL
    LIBRARIES
        hio
        glf
        garch
        tf
    CPPFILES
        testenv/testHdStGLSL.cpp
)
pxr_build_test(testHdStHWFaceCulling
    LIBRARIES
        hdSt
        hd
        glf
        tf
        gf
    CPPFILES
        testenv/testHdStHWFaceCulling.cpp
)
pxr_build_test(testHdStIndirectDrawBatchCodeGen
    LIBRARIES
        hdSt
        hd
        hio
        glf
        tf
    CPPFILES
        testenv/testHdStIndirectDrawBatchCodeGen.cpp
)
pxr_build_test(testHdStIndirectDrawBatchCulling
    LIBRARIES
        hdSt
        glf
        tf
    CPPFILES
        testenv/testHdStIndirectDrawBatchCulling.cpp
)
pxr_build_test(testHdStInitRepr
    LIBRARIES
        hdSt
        hd
        tf
        gf
    CPPFILES
        testenv/testHdStInitRepr.cpp
)
pxr_build_test(testHdStInstancing
    LIBRARIES
        hdSt
        tf
        gf
    CPPFILES
        testenv/testHdStInstancing.cpp
)
pxr_build_test(testHdStInstancingUnbalanced
    LIBRARIES
        hdSt
        tf
        gf
    CPPFILES
        testenv/testHdStInstancingUnbalanced.cpp
)
if (${PXR_ENABLE_MATERIALX_SUPPORT})
pxr_build_test(testHdStMaterialXShaderGen
    LIBRARIES
        hdSt
        hdMtlx
        tf
    CPPFILES
        testenv/testHdStMaterialXShaderGen.cpp
)
endif()
pxr_build_test(testHdStMeshTopology
    LIBRARIES
        hdSt
        hd
        sdf
        glf
        tf
        gf
    CPPFILES
        testenv/testHdStMeshTopology.cpp
)
pxr_build_test(testHdStMultipleFvarTopologies
    LIBRARIES
        hdSt
        tf
        gf
    CPPFILES
        testenv/testHdStMultipleFvarTopologies.cpp
)
pxr_build_test(testHdStPrimGather
    LIBRARIES
        hdSt
        hd
        tf
        gf
    CPPFILES
        testenv/testHdStPrimGather.cpp
)
pxr_build_test(testHdStPrimitiveParam
    LIBRARIES
        hdSt
        hd
        hgi
        glf
        sdf
        tf
    CPPFILES
        testenv/testHdStPrimitiveParam.cpp
)
pxr_build_test(testHdStPrimvars
    LIBRARIES
        hdSt
        hd
        glf
        sdf
        tf
        gf
    CPPFILES
        testenv/testHdStPrimvars.cpp
)
pxr_build_test(testHdStQuadrangulation
    LIBRARIES
        hdSt
        hd
        glf
        tf
        gf
    CPPFILES
        testenv/testHdStQuadrangulation.cpp
)
pxr_build_test(testHdStRenderPass
    LIBRARIES
        hdSt
        hd
        glf
        tf
    CPPFILES
        testenv/testHdStRenderPass.cpp
)
pxr_build_test(testHdStSamplerObjectRegistry
    LIBRARIES
        hdSt
        tf
    CPPFILES
        testenv/testHdStSamplerObjectRegistry.cpp
)
pxr_build_test(testHdStSubdivision
    LIBRARIES
        hdSt
        hd
        pxOsd
        hgi
        glf
        tf
        gf
    CPPFILES
        testenv/testHdStSubdivision.cpp
)
pxr_build_test(testHdStTextureHandleRegistry
    LIBRARIES
        hdSt
        tf
    CPPFILES
        testenv/testHdStTextureHandleRegistry.cpp
)
pxr_build_test(testHdStTextureObjectRegistry
    LIBRARIES
        hdSt
        tf
    CPPFILES
        testenv/testHdStTextureObjectRegistry.cpp
)
pxr_build_test(testHdStTransition
    LIBRARIES
        hdSt
        hd
        tf
        gf
    CPPFILES
        testenv/testHdStTransition.cpp
)
pxr_build_test(testHdStSubResourceRegistry
    LIBRARIES
        garch
        gf
        hd
        hdSt
        hgi
        tf
        vt
    CPPFILES
        testenv/testHdStSubResourceRegistry.cpp
)
endif()

if (APPLE)
    message(STATUS "Skipping ${PXR_PACKAGE} tests because they are currently unsupported on macOS")
    return()
endif()

if (WIN32)
    message(STATUS "Skipping ${PXR_PACKAGE} tests because they are currently unsupported on Windows")
    return()
endif()

# Most HdSt tests require setup of a GL context and window. This is the 
# exception.
pxr_register_test(testHdStBufferSource
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBufferSource"
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBufferSource
    ENV
        TF_DEBUG=HD_SAFE_MODE
)

if (${PXR_HEADLESS_TEST_MODE})
    message(STATUS "Skipping ${PXR_PACKAGE} tests because PXR_HEADLESS_TEST_MODE is ON")
    return()
endif()

#
# Install tests
#
pxr_install_test_dir(
    SRC testenv/testHdStBarAllocationLimit
    DEST testHdStBarAllocationLimit
)
pxr_install_test_dir(
    SRC testenv/testHdStBasicDrawing_Adaptive
    DEST testHdStBasicDrawing_Adaptive
)
pxr_install_test_dir(
    SRC testenv/testHdStBasicDrawing
    DEST testHdStBasicDrawing
)
pxr_install_test_dir(
    SRC testenv/testHdStBasicDrawing_Quadrangulate
    DEST testHdStBasicDrawing_Quadrangulate
)
pxr_install_test_dir(
    SRC testenv/testHdStBufferArray
    DEST testHdStBufferArray
)
pxr_install_test_dir(
    SRC testenv/testHdStBufferArrayInstancingDisabled
    DEST testHdStBufferArrayInstancingDisabled
)
pxr_install_test_dir(
    SRC testenv/testHdStClipPlanes
    DEST testHdStClipPlanes
)
pxr_install_test_dir(
    SRC testenv/testHdStCodeGen
    DEST testHdStCodeGen
)
pxr_install_test_dir(
    SRC testenv/testHdStCurveDrawing
    DEST testHdStCurveDrawing
)
pxr_install_test_dir(
    SRC testenv/testHdStCurvePrimvarInterpolation
    DEST testHdStCurvePrimvarInterpolation
)
pxr_install_test_dir(
    SRC testenv/testHdStDisplayStyle
    DEST testHdStDisplayStyle
)
pxr_install_test_dir(
    SRC testenv/testHdStDrawBatching
    DEST testHdStDrawBatching
)
pxr_install_test_dir(
    SRC testenv/testHdStDynamicUvTexture
    DEST testHdStDynamicUvTexture
)
pxr_install_test_dir(
    SRC testenv/testHdStFaceCulling
    DEST testHdStFaceCulling
)
pxr_install_test_dir(
    SRC testenv/testHdStGLSL
    DEST testHdStGLSL
)
pxr_install_test_dir(
    SRC testenv/testHdStHWFaceCulling
    DEST testHdStHWFaceCulling
)
pxr_install_test_dir(
    SRC testenv/testHdStInitRepr
    DEST testHdStInitRepr
)
pxr_install_test_dir(
    SRC testenv/testHdStInstancing
    DEST testHdStInstancing
)
pxr_install_test_dir(
    SRC testenv/testHdStInstancingUnbalanced
    DEST testHdStInstancingUnbalanced
)
pxr_install_test_dir(
    SRC testenv/testHdStInstancingUnbalancedNoBindless
    DEST testHdStInstancingUnbalancedNoBindless
)
if (${PXR_ENABLE_MATERIALX_SUPPORT})
pxr_install_test_dir(
    SRC testenv/testHdStMaterialXShaderGen
    DEST testHdStMaterialXShaderGen
)
endif()
pxr_install_test_dir(
    SRC testenv/testHdStMultipleFvarTopologies
    DEST testHdStMultipleFvarTopologies
)
pxr_install_test_dir(
    SRC testenv/testHdStPrimGather
    DEST testHdStPrimGather
)
pxr_install_test_dir(
    SRC testenv/testHdStPrimvars
    DEST testHdStPrimvars
)
pxr_install_test_dir(
    SRC testenv/testHdStSamplerObjectRegistry
    DEST testHdStSamplerObjectRegistry
)
pxr_install_test_dir(
    SRC testenv/testHdStSubdivisionOsd3
    DEST testHdStSubdivisionOsd3
)
pxr_install_test_dir(
    SRC testenv/testHdStTextureHandleRegistry
    DEST testHdStTextureHandleRegistry
)
pxr_install_test_dir(
    SRC testenv/testHdStTextureObjectRegistry
    DEST testHdStTextureObjectRegistry
)
pxr_install_test_dir(
    SRC testenv/testHdStTransition
    DEST testHdStTransition
)
pxr_install_test_dir(
    SRC testenv/testHdStTransitionRefinedCurves
    DEST testHdStTransitionRefinedCurves
)

#
# Register tests
#
pxr_register_test(testHdStBarAllocationLimit
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBarAllocationLimit --offscreen --write testHdStBarAllocationLimit.png"
    EXPECTED_RETURN_CODE 0
    IMAGE_DIFF_COMPARE
        testHdStBarAllocationLimit.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    TESTENV testHdStBarAllocationLimit
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_Adaptive_lighting_lv1_hull
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --lighting --repr hull --refineLevel 1 --write testHdStBasicDrawing_adaptive_lighting_lv1_hull.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_adaptive_lighting_lv1_hull.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing_Adaptive
    ENV
        HD_ENABLE_OPENSUBDIV3_ADAPTIVE=1
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_Adaptive_lighting_lv1_smoothHull
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --lighting --repr smoothHull --refineLevel 1 --write testHdStBasicDrawing_adaptive_lighting_lv1_smoothHull.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_adaptive_lighting_lv1_smoothHull.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing_Adaptive
    ENV
        HD_ENABLE_OPENSUBDIV3_ADAPTIVE=1
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_Adaptive_lighting_lv1_wire
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --lighting --repr wire --refineLevel 1 --write testHdStBasicDrawing_adaptive_lighting_lv1_wire.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_adaptive_lighting_lv1_wire.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing_Adaptive
    ENV
        HD_ENABLE_OPENSUBDIV3_ADAPTIVE=1
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_Adaptive_lighting_lv1_wireOnSurf
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --lighting --repr wireOnSurf --refineLevel 1 --write testHdStBasicDrawing_adaptive_lighting_lv1_wireOnSurf.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_adaptive_lighting_lv1_wireOnSurf.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing_Adaptive
    ENV
        HD_ENABLE_OPENSUBDIV3_ADAPTIVE=1
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_Adaptive_lighting_lv1_refined
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --lighting --repr refined --refineLevel 1 --write testHdStBasicDrawing_adaptive_lighting_lv1_refined.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_adaptive_lighting_lv1_refined.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing_Adaptive
    ENV
        HD_ENABLE_OPENSUBDIV3_ADAPTIVE=1
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_Adaptive_lighting_lv1_refinedWire
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --lighting --repr refinedWire --refineLevel 1 --write testHdStBasicDrawing_adaptive_lighting_lv1_refinedWire.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_adaptive_lighting_lv1_refinedWire.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing_Adaptive
    ENV
        HD_ENABLE_OPENSUBDIV3_ADAPTIVE=1
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_Adaptive_lighting_lv1_refinedWireOnSurf
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --lighting --repr refinedWireOnSurf --refineLevel 1 --write testHdStBasicDrawing_adaptive_lighting_lv1_refinedWireOnSurf.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_adaptive_lighting_lv1_refinedWireOnSurf.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing_Adaptive
    ENV
        HD_ENABLE_OPENSUBDIV3_ADAPTIVE=1
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_Adaptive_lighting_lv2_hull
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --lighting --repr hull --refineLevel 2 --write testHdStBasicDrawing_adaptive_lighting_lv2_hull.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_adaptive_lighting_lv2_hull.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing_Adaptive
    ENV
        HD_ENABLE_OPENSUBDIV3_ADAPTIVE=1
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_Adaptive_lighting_lv2_smoothHull
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --lighting --repr smoothHull --refineLevel 2 --write testHdStBasicDrawing_adaptive_lighting_lv2_smoothHull.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_adaptive_lighting_lv2_smoothHull.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing_Adaptive
    ENV
        HD_ENABLE_OPENSUBDIV3_ADAPTIVE=1
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_Adaptive_lighting_lv2_wire
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --lighting --repr wire --refineLevel 2 --write testHdStBasicDrawing_adaptive_lighting_lv2_wire.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_adaptive_lighting_lv2_wire.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing_Adaptive
    ENV
        HD_ENABLE_OPENSUBDIV3_ADAPTIVE=1
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_Adaptive_lighting_lv2_wireOnSurf
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --lighting --repr wireOnSurf --refineLevel 2 --write testHdStBasicDrawing_adaptive_lighting_lv2_wireOnSurf.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_adaptive_lighting_lv2_wireOnSurf.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing_Adaptive
    ENV
        HD_ENABLE_OPENSUBDIV3_ADAPTIVE=1
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_Adaptive_lighting_lv2_refined
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --lighting --repr refined --refineLevel 2 --write testHdStBasicDrawing_adaptive_lighting_lv2_refined.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_adaptive_lighting_lv2_refined.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing_Adaptive
    ENV
        HD_ENABLE_OPENSUBDIV3_ADAPTIVE=1
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_Adaptive_lighting_lv2_refinedWire
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --lighting --repr refinedWire --refineLevel 2 --write testHdStBasicDrawing_adaptive_lighting_lv2_refinedWire.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_adaptive_lighting_lv2_refinedWire.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing_Adaptive
    ENV
        HD_ENABLE_OPENSUBDIV3_ADAPTIVE=1
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_Adaptive_lighting_lv2_refinedWireOnSurf
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --lighting --repr refinedWireOnSurf --refineLevel 2 --write testHdStBasicDrawing_adaptive_lighting_lv2_refinedWireOnSurf.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_adaptive_lighting_lv2_refinedWireOnSurf.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing_Adaptive
    ENV
        HD_ENABLE_OPENSUBDIV3_ADAPTIVE=1
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_Immediate_lv0_points
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --repr points --write testHdStBasicDrawing_lv0_points.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lv0_points.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        HGIGL_ENABLE_MULTI_DRAW_INDIRECT=0
        HGIVULKAN_ENABLE_MULTI_DRAW_INDIRECT=0
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_Immediate_lv0_hull
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --repr hull --write testHdStBasicDrawing_lv0_hull.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lv0_hull.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        HGIGL_ENABLE_MULTI_DRAW_INDIRECT=0
        HGIVULKAN_ENABLE_MULTI_DRAW_INDIRECT=0
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_Immediate_lv0_smoothHull
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --repr smoothHull --write testHdStBasicDrawing_lv0_smoothHull.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lv0_smoothHull.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        HGIGL_ENABLE_MULTI_DRAW_INDIRECT=0
        HGIVULKAN_ENABLE_MULTI_DRAW_INDIRECT=0
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_Immediate_lv0_wire
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --repr wire --write testHdStBasicDrawing_lv0_wire.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lv0_wire.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        HGIGL_ENABLE_MULTI_DRAW_INDIRECT=0
        HGIVULKAN_ENABLE_MULTI_DRAW_INDIRECT=0
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_Immediate_lv0_wireOnSurf
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --repr wireOnSurf --write testHdStBasicDrawing_lv0_wireOnSurf.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lv0_wireOnSurf.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        HGIGL_ENABLE_MULTI_DRAW_INDIRECT=0
        HGIVULKAN_ENABLE_MULTI_DRAW_INDIRECT=0
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_Immediate_lv0_hull_cullback
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --repr hull --cullStyle BackUnlessDoubleSided --write testHdStBasicDrawing_lv0_hull_cullback.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lv0_hull_cullback.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        HGIGL_ENABLE_MULTI_DRAW_INDIRECT=0
        HGIVULKAN_ENABLE_MULTI_DRAW_INDIRECT=0
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_Immediate_lv0_smoothHull_cullback
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --repr smoothHull --cullStyle BackUnlessDoubleSided --write testHdStBasicDrawing_lv0_smoothHull_cullback.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lv0_smoothHull_cullback.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        HGIGL_ENABLE_MULTI_DRAW_INDIRECT=0
        HGIVULKAN_ENABLE_MULTI_DRAW_INDIRECT=0
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_Immediate_lv0_wire_cullback
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --repr wire --cullStyle BackUnlessDoubleSided --write testHdStBasicDrawing_lv0_wire_cullback.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lv0_wire_cullback.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        HGIGL_ENABLE_MULTI_DRAW_INDIRECT=0
        HGIVULKAN_ENABLE_MULTI_DRAW_INDIRECT=0
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_Immediate_lv0_wireOnSurf_cullback
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --repr wireOnSurf --cullStyle BackUnlessDoubleSided --write testHdStBasicDrawing_lv0_wireOnSurf_cullback.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lv0_wireOnSurf_cullback.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        HGIGL_ENABLE_MULTI_DRAW_INDIRECT=0
        HGIVULKAN_ENABLE_MULTI_DRAW_INDIRECT=0
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_Immediate_lv0_hull_cullfront
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --repr hull --cullStyle FrontUnlessDoubleSided --write testHdStBasicDrawing_lv0_hull_cullfront.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lv0_hull_cullfront.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        HGIGL_ENABLE_MULTI_DRAW_INDIRECT=0
        HGIVULKAN_ENABLE_MULTI_DRAW_INDIRECT=0
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_Immediate_lv0_smoothHull_cullfront
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --repr smoothHull --cullStyle FrontUnlessDoubleSided --write testHdStBasicDrawing_lv0_smoothHull_cullfront.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lv0_smoothHull_cullfront.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        HGIGL_ENABLE_MULTI_DRAW_INDIRECT=0
        HGIVULKAN_ENABLE_MULTI_DRAW_INDIRECT=0
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_Immediate_lv0_wire_cullfront
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --repr wire --cullStyle FrontUnlessDoubleSided --write testHdStBasicDrawing_lv0_wire_cullfront.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lv0_wire_cullfront.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        HGIGL_ENABLE_MULTI_DRAW_INDIRECT=0
        HGIVULKAN_ENABLE_MULTI_DRAW_INDIRECT=0
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_Immediate_lv0_wireOnSurf_cullfront
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --repr wireOnSurf --cullStyle FrontUnlessDoubleSided --write testHdStBasicDrawing_lv0_wireOnSurf_cullfront.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lv0_wireOnSurf_cullfront.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        HGIGL_ENABLE_MULTI_DRAW_INDIRECT=0
        HGIVULKAN_ENABLE_MULTI_DRAW_INDIRECT=0
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_Immediate_lv1_points
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --repr points --refineLevel 1 --write testHdStBasicDrawing_lv1_points.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lv1_points.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        HGIGL_ENABLE_MULTI_DRAW_INDIRECT=0
        HGIVULKAN_ENABLE_MULTI_DRAW_INDIRECT=0
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_Immediate_lv1_hull
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --repr hull --refineLevel 1 --write testHdStBasicDrawing_lv1_hull.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lv1_hull.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        HGIGL_ENABLE_MULTI_DRAW_INDIRECT=0
        HGIVULKAN_ENABLE_MULTI_DRAW_INDIRECT=0
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_Immediate_lv1_smoothHull
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --repr smoothHull --refineLevel 1 --write testHdStBasicDrawing_lv1_smoothHull.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lv1_smoothHull.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        HGIGL_ENABLE_MULTI_DRAW_INDIRECT=0
        HGIVULKAN_ENABLE_MULTI_DRAW_INDIRECT=0
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_Immediate_lv1_wire
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --repr wire --refineLevel 1 --write testHdStBasicDrawing_lv1_wire.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lv1_wire.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        HGIGL_ENABLE_MULTI_DRAW_INDIRECT=0
        HGIVULKAN_ENABLE_MULTI_DRAW_INDIRECT=0
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_Immediate_lv1_wireOnSurf
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --repr wireOnSurf --refineLevel 1 --write testHdStBasicDrawing_lv1_wireOnSurf.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lv1_wireOnSurf.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        HGIGL_ENABLE_MULTI_DRAW_INDIRECT=0
        HGIVULKAN_ENABLE_MULTI_DRAW_INDIRECT=0
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_Immediate_lv1_refined
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --repr refined --refineLevel 1 --write testHdStBasicDrawing_lv1_refined.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lv1_refined.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        HGIGL_ENABLE_MULTI_DRAW_INDIRECT=0
        HGIVULKAN_ENABLE_MULTI_DRAW_INDIRECT=0
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_Immediate_lv1_refinedWire
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --repr refinedWire --refineLevel 1 --write testHdStBasicDrawing_lv1_refinedWire.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lv1_refinedWire.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        HGIGL_ENABLE_MULTI_DRAW_INDIRECT=0
        HGIVULKAN_ENABLE_MULTI_DRAW_INDIRECT=0
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_Immediate_lv1_refinedWireOnSurf
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --repr refinedWireOnSurf --refineLevel 1 --write testHdStBasicDrawing_lv1_refinedWireOnSurf.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lv1_refinedWireOnSurf.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        HGIGL_ENABLE_MULTI_DRAW_INDIRECT=0
        HGIVULKAN_ENABLE_MULTI_DRAW_INDIRECT=0
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_Immediate_lv2_hull
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --repr hull --refineLevel 2 --write testHdStBasicDrawing_lv2_hull.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lv2_hull.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        HGIGL_ENABLE_MULTI_DRAW_INDIRECT=0
        HGIVULKAN_ENABLE_MULTI_DRAW_INDIRECT=0
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_Immediate_lv2_smoothHull
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --repr smoothHull --refineLevel 2 --write testHdStBasicDrawing_lv2_smoothHull.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lv2_smoothHull.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        HGIGL_ENABLE_MULTI_DRAW_INDIRECT=0
        HGIVULKAN_ENABLE_MULTI_DRAW_INDIRECT=0
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_Immediate_lv2_wire
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --repr wire --refineLevel 2 --write testHdStBasicDrawing_lv2_wire.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lv2_wire.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        HGIGL_ENABLE_MULTI_DRAW_INDIRECT=0
        HGIVULKAN_ENABLE_MULTI_DRAW_INDIRECT=0
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_Immediate_lv2_wireOnSurf
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --repr wireOnSurf --refineLevel 2 --write testHdStBasicDrawing_lv2_wireOnSurf.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lv2_wireOnSurf.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        HGIGL_ENABLE_MULTI_DRAW_INDIRECT=0
        HGIVULKAN_ENABLE_MULTI_DRAW_INDIRECT=0
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_Immediate_lv2_refined
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --repr refined --refineLevel 2 --write testHdStBasicDrawing_lv2_refined.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lv2_refined.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        HGIGL_ENABLE_MULTI_DRAW_INDIRECT=0
        HGIVULKAN_ENABLE_MULTI_DRAW_INDIRECT=0
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_Immediate_lv2_refinedWire
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --repr refinedWire --refineLevel 2 --write testHdStBasicDrawing_lv2_refinedWire.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lv2_refinedWire.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        HGIGL_ENABLE_MULTI_DRAW_INDIRECT=0
        HGIVULKAN_ENABLE_MULTI_DRAW_INDIRECT=0
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_Immediate_lv2_refinedWireOnSurf
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --repr refinedWireOnSurf --refineLevel 2 --write testHdStBasicDrawing_lv2_refinedWireOnSurf.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lv2_refinedWireOnSurf.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        HGIGL_ENABLE_MULTI_DRAW_INDIRECT=0
        HGIVULKAN_ENABLE_MULTI_DRAW_INDIRECT=0
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_Immediate_lighting_lv0_hull
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --lighting --repr hull --write testHdStBasicDrawing_lighting_lv0_hull.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lighting_lv0_hull.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        HGIGL_ENABLE_MULTI_DRAW_INDIRECT=0
        HGIVULKAN_ENABLE_MULTI_DRAW_INDIRECT=0
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_Immediate_lighting_lv0_smoothHull
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --lighting --repr smoothHull --write testHdStBasicDrawing_lighting_lv0_smoothHull.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lighting_lv0_smoothHull.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        HGIGL_ENABLE_MULTI_DRAW_INDIRECT=0
        HGIVULKAN_ENABLE_MULTI_DRAW_INDIRECT=0
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_Immediate_lighting_lv0_wire
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --lighting --repr wire --write testHdStBasicDrawing_lighting_lv0_wire.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lighting_lv0_wire.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        HGIGL_ENABLE_MULTI_DRAW_INDIRECT=0
        HGIVULKAN_ENABLE_MULTI_DRAW_INDIRECT=0
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_Immediate_lighting_lv0_wireOnSurf
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --lighting --repr wireOnSurf --write testHdStBasicDrawing_lighting_lv0_wireOnSurf.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lighting_lv0_wireOnSurf.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        HGIGL_ENABLE_MULTI_DRAW_INDIRECT=0
        HGIVULKAN_ENABLE_MULTI_DRAW_INDIRECT=0
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_Immediate_lighting_lv1_hull
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --lighting --repr hull --refineLevel 1 --write testHdStBasicDrawing_lighting_lv1_hull.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lighting_lv1_hull.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        HGIGL_ENABLE_MULTI_DRAW_INDIRECT=0
        HGIVULKAN_ENABLE_MULTI_DRAW_INDIRECT=0
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_Immediate_lighting_lv1_smoothHull
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --lighting --repr smoothHull --refineLevel 1 --write testHdStBasicDrawing_lighting_lv1_smoothHull.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lighting_lv1_smoothHull.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        HGIGL_ENABLE_MULTI_DRAW_INDIRECT=0
        HGIVULKAN_ENABLE_MULTI_DRAW_INDIRECT=0
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_Immediate_lighting_lv1_wire
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --lighting --repr wire --refineLevel 1 --write testHdStBasicDrawing_lighting_lv1_wire.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lighting_lv1_wire.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        HGIGL_ENABLE_MULTI_DRAW_INDIRECT=0
        HGIVULKAN_ENABLE_MULTI_DRAW_INDIRECT=0
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_Immediate_lighting_lv1_wireOnSurf
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --lighting --repr wireOnSurf --refineLevel 1 --write testHdStBasicDrawing_lighting_lv1_wireOnSurf.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lighting_lv1_wireOnSurf.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        HGIGL_ENABLE_MULTI_DRAW_INDIRECT=0
        HGIVULKAN_ENABLE_MULTI_DRAW_INDIRECT=0
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_Immediate_lighting_lv1_refined
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --lighting --repr refined --refineLevel 1 --write testHdStBasicDrawing_lighting_lv1_refined.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lighting_lv1_refined.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        HGIGL_ENABLE_MULTI_DRAW_INDIRECT=0
        HGIVULKAN_ENABLE_MULTI_DRAW_INDIRECT=0
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_Immediate_lighting_lv1_refinedWire
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --lighting --repr refinedWire --refineLevel 1 --write testHdStBasicDrawing_lighting_lv1_refinedWire.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lighting_lv1_refinedWire.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        HGIGL_ENABLE_MULTI_DRAW_INDIRECT=0
        HGIVULKAN_ENABLE_MULTI_DRAW_INDIRECT=0
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_Immediate_lighting_lv1_refinedWireOnSurf
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --lighting --repr refinedWireOnSurf --refineLevel 1 --write testHdStBasicDrawing_lighting_lv1_refinedWireOnSurf.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lighting_lv1_refinedWireOnSurf.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        HGIGL_ENABLE_MULTI_DRAW_INDIRECT=0
        HGIVULKAN_ENABLE_MULTI_DRAW_INDIRECT=0
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_Immediate_lighting_lv2_hull
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --lighting --repr hull --refineLevel 2 --write testHdStBasicDrawing_lighting_lv2_hull.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lighting_lv2_hull.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        HGIGL_ENABLE_MULTI_DRAW_INDIRECT=0
        HGIVULKAN_ENABLE_MULTI_DRAW_INDIRECT=0
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_Immediate_lighting_lv2_smoothHull
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --lighting --repr smoothHull --refineLevel 2 --write testHdStBasicDrawing_lighting_lv2_smoothHull.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lighting_lv2_smoothHull.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        HGIGL_ENABLE_MULTI_DRAW_INDIRECT=0
        HGIVULKAN_ENABLE_MULTI_DRAW_INDIRECT=0
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_Immediate_lighting_lv2_wire
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --lighting --repr wire --refineLevel 2 --write testHdStBasicDrawing_lighting_lv2_wire.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lighting_lv2_wire.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        HGIGL_ENABLE_MULTI_DRAW_INDIRECT=0
        HGIVULKAN_ENABLE_MULTI_DRAW_INDIRECT=0
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_Immediate_lighting_lv2_wireOnSurf
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --lighting --repr wireOnSurf --refineLevel 2 --write testHdStBasicDrawing_lighting_lv2_wireOnSurf.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lighting_lv2_wireOnSurf.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        HGIGL_ENABLE_MULTI_DRAW_INDIRECT=0
        HGIVULKAN_ENABLE_MULTI_DRAW_INDIRECT=0
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_Immediate_lighting_lv2_refined
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --lighting --repr refined --refineLevel 2 --write testHdStBasicDrawing_lighting_lv2_refined.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lighting_lv2_refined.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        HGIGL_ENABLE_MULTI_DRAW_INDIRECT=0
        HGIVULKAN_ENABLE_MULTI_DRAW_INDIRECT=0
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_Immediate_lighting_lv2_refinedWire
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --lighting --repr refinedWire --refineLevel 2 --write testHdStBasicDrawing_lighting_lv2_refinedWire.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lighting_lv2_refinedWire.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        HGIGL_ENABLE_MULTI_DRAW_INDIRECT=0
        HGIVULKAN_ENABLE_MULTI_DRAW_INDIRECT=0
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_Immediate_lighting_lv2_refinedWireOnSurf
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --lighting --repr refinedWireOnSurf --refineLevel 2 --write testHdStBasicDrawing_lighting_lv2_refinedWireOnSurf.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lighting_lv2_refinedWireOnSurf.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        HGIGL_ENABLE_MULTI_DRAW_INDIRECT=0
        HGIVULKAN_ENABLE_MULTI_DRAW_INDIRECT=0
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_MDI_lv0_points
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --repr points --write testHdStBasicDrawing_lv0_points.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lv0_points.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_MDI_lv0_hull
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --repr hull --write testHdStBasicDrawing_lv0_hull.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lv0_hull.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_MDI_lv0_smoothHull
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --repr smoothHull --write testHdStBasicDrawing_lv0_smoothHull.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lv0_smoothHull.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_MDI_lv0_wire
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --repr wire --write testHdStBasicDrawing_lv0_wire.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lv0_wire.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_MDI_lv0_wireOnSurf
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --repr wireOnSurf --write testHdStBasicDrawing_lv0_wireOnSurf.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lv0_wireOnSurf.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_MDI_lv0_hull_cullback
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --repr hull --cullStyle BackUnlessDoubleSided --write testHdStBasicDrawing_lv0_hull_cullback.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lv0_hull_cullback.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_MDI_lv0_smoothHull_cullback
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --repr smoothHull --cullStyle BackUnlessDoubleSided --write testHdStBasicDrawing_lv0_smoothHull_cullback.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lv0_smoothHull_cullback.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_MDI_lv0_wire_cullback
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --repr wire --cullStyle BackUnlessDoubleSided --write testHdStBasicDrawing_lv0_wire_cullback.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lv0_wire_cullback.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_MDI_lv0_wireOnSurf_cullback
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --repr wireOnSurf --cullStyle BackUnlessDoubleSided --write testHdStBasicDrawing_lv0_wireOnSurf_cullback.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lv0_wireOnSurf_cullback.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_MDI_lv0_hull_cullfront
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --repr hull --cullStyle FrontUnlessDoubleSided --write testHdStBasicDrawing_lv0_hull_cullfront.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lv0_hull_cullfront.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_MDI_lv0_smoothHull_cullfront
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --repr smoothHull --cullStyle FrontUnlessDoubleSided --write testHdStBasicDrawing_lv0_smoothHull_cullfront.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lv0_smoothHull_cullfront.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_MDI_lv0_wire_cullfront
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --repr wire --cullStyle FrontUnlessDoubleSided --write testHdStBasicDrawing_lv0_wire_cullfront.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lv0_wire_cullfront.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_MDI_lv0_wireOnSurf_cullfront
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --repr wireOnSurf --cullStyle FrontUnlessDoubleSided --write testHdStBasicDrawing_lv0_wireOnSurf_cullfront.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lv0_wireOnSurf_cullfront.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_MDI_lv1_points
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --repr points --refineLevel 1 --write testHdStBasicDrawing_lv1_points.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lv1_points.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_MDI_lv1_hull
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --repr hull --refineLevel 1 --write testHdStBasicDrawing_lv1_hull.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lv1_hull.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_MDI_lv1_smoothHull
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --repr smoothHull --refineLevel 1 --write testHdStBasicDrawing_lv1_smoothHull.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lv1_smoothHull.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_MDI_lv1_wire
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --repr wire --refineLevel 1 --write testHdStBasicDrawing_lv1_wire.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lv1_wire.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_MDI_lv1_wireOnSurf
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --repr wireOnSurf --refineLevel 1 --write testHdStBasicDrawing_lv1_wireOnSurf.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lv1_wireOnSurf.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_MDI_lv1_refined
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --repr refined --refineLevel 1 --write testHdStBasicDrawing_lv1_refined.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lv1_refined.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_MDI_lv1_refinedWire
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --repr refinedWire --refineLevel 1 --write testHdStBasicDrawing_lv1_refinedWire.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lv1_refinedWire.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_MDI_lv1_refinedWireOnSurf
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --repr refinedWireOnSurf --refineLevel 1 --write testHdStBasicDrawing_lv1_refinedWireOnSurf.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lv1_refinedWireOnSurf.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_MDI_lv2_hull
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --repr hull --refineLevel 2 --write testHdStBasicDrawing_lv2_hull.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lv2_hull.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_MDI_lv2_smoothHull
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --repr smoothHull --refineLevel 2 --write testHdStBasicDrawing_lv2_smoothHull.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lv2_smoothHull.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_MDI_lv2_wire
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --repr wire --refineLevel 2 --write testHdStBasicDrawing_lv2_wire.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lv2_wire.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_MDI_lv2_wireOnSurf
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --repr wireOnSurf --refineLevel 2 --write testHdStBasicDrawing_lv2_wireOnSurf.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lv2_wireOnSurf.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_MDI_lv2_refined
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --repr refined --refineLevel 2 --write testHdStBasicDrawing_lv2_refined.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lv2_refined.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_MDI_lv2_refinedWire
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --repr refinedWire --refineLevel 2 --write testHdStBasicDrawing_lv2_refinedWire.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lv2_refinedWire.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_MDI_lv2_refinedWireOnSurf
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --repr refinedWireOnSurf --refineLevel 2 --write testHdStBasicDrawing_lv2_refinedWireOnSurf.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lv2_refinedWireOnSurf.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_MDI_lighting_lv0_hull
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --lighting --repr hull --write testHdStBasicDrawing_lighting_lv0_hull.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lighting_lv0_hull.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_MDI_lighting_lv0_smoothHull
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --lighting --repr smoothHull --write testHdStBasicDrawing_lighting_lv0_smoothHull.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lighting_lv0_smoothHull.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_MDI_lighting_lv0_wire
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --lighting --repr wire --write testHdStBasicDrawing_lighting_lv0_wire.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lighting_lv0_wire.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_MDI_lighting_lv0_wireOnSurf
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --lighting --repr wireOnSurf --write testHdStBasicDrawing_lighting_lv0_wireOnSurf.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lighting_lv0_wireOnSurf.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_MDI_lighting_lv1_hull
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --lighting --repr hull --refineLevel 1 --write testHdStBasicDrawing_lighting_lv1_hull.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lighting_lv1_hull.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_MDI_lighting_lv1_smoothHull
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --lighting --repr smoothHull --refineLevel 1 --write testHdStBasicDrawing_lighting_lv1_smoothHull.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lighting_lv1_smoothHull.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_MDI_lighting_lv1_wire
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --lighting --repr wire --refineLevel 1 --write testHdStBasicDrawing_lighting_lv1_wire.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lighting_lv1_wire.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_MDI_lighting_lv1_wireOnSurf
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --lighting --repr wireOnSurf --refineLevel 1 --write testHdStBasicDrawing_lighting_lv1_wireOnSurf.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lighting_lv1_wireOnSurf.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_MDI_lighting_lv1_refined
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --lighting --repr refined --refineLevel 1 --write testHdStBasicDrawing_lighting_lv1_refined.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lighting_lv1_refined.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_MDI_lighting_lv1_refinedWire
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --lighting --repr refinedWire --refineLevel 1 --write testHdStBasicDrawing_lighting_lv1_refinedWire.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lighting_lv1_refinedWire.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_MDI_lighting_lv1_refinedWireOnSurf
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --lighting --repr refinedWireOnSurf --refineLevel 1 --write testHdStBasicDrawing_lighting_lv1_refinedWireOnSurf.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lighting_lv1_refinedWireOnSurf.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_MDI_lighting_lv2_hull
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --lighting --repr hull --refineLevel 2 --write testHdStBasicDrawing_lighting_lv2_hull.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lighting_lv2_hull.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_MDI_lighting_lv2_smoothHull
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --lighting --repr smoothHull --refineLevel 2 --write testHdStBasicDrawing_lighting_lv2_smoothHull.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lighting_lv2_smoothHull.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_MDI_lighting_lv2_wire
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --lighting --repr wire --refineLevel 2 --write testHdStBasicDrawing_lighting_lv2_wire.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lighting_lv2_wire.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_MDI_lighting_lv2_wireOnSurf
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --lighting --repr wireOnSurf --refineLevel 2 --write testHdStBasicDrawing_lighting_lv2_wireOnSurf.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lighting_lv2_wireOnSurf.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_MDI_lighting_lv2_refined
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --lighting --repr refined --refineLevel 2 --write testHdStBasicDrawing_lighting_lv2_refined.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lighting_lv2_refined.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_MDI_lighting_lv2_refinedWire
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --lighting --repr refinedWire --refineLevel 2 --write testHdStBasicDrawing_lighting_lv2_refinedWire.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lighting_lv2_refinedWire.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_MDI_lighting_lv2_refinedWireOnSurf
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --lighting --repr refinedWireOnSurf --refineLevel 2 --write testHdStBasicDrawing_lighting_lv2_refinedWireOnSurf.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_lighting_lv2_refinedWireOnSurf.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_Quadrangulate_hull
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --repr hull --write testHdStBasicDrawing_quad_hull.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_quad_hull.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing_Quadrangulate
    ENV
        HD_ENABLE_FORCE_QUADRANGULATE=1
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_Quadrangulate_smoothHull
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --repr smoothHull --write testHdStBasicDrawing_quad_smoothHull.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_quad_smoothHull.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing_Quadrangulate
    ENV
        HD_ENABLE_FORCE_QUADRANGULATE=1
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_Quadrangulate_wire
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --repr wire --write testHdStBasicDrawing_quad_wire.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_quad_wire.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing_Quadrangulate
    ENV
        HD_ENABLE_FORCE_QUADRANGULATE=1
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_Quadrangulate_wireOnSurf
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --repr wireOnSurf --write testHdStBasicDrawing_quad_wireOnSurf.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_quad_wireOnSurf.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing_Quadrangulate
    ENV
        HD_ENABLE_FORCE_QUADRANGULATE=1
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_Quadrangulate_hull_cullback
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --repr hull --cullStyle BackUnlessDoubleSided --write testHdStBasicDrawing_quad_hull_cullback.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_quad_hull_cullback.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing_Quadrangulate
    ENV
        HD_ENABLE_FORCE_QUADRANGULATE=1
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_Quadrangulate_smoothHull_cullback
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --repr smoothHull --cullStyle BackUnlessDoubleSided --write testHdStBasicDrawing_quad_smoothHull_cullback.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_quad_smoothHull_cullback.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing_Quadrangulate
    ENV
        HD_ENABLE_FORCE_QUADRANGULATE=1
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_Quadrangulate_wire_cullback
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --repr wire --cullStyle BackUnlessDoubleSided --write testHdStBasicDrawing_quad_wire_cullback.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_quad_wire_cullback.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing_Quadrangulate
    ENV
        HD_ENABLE_FORCE_QUADRANGULATE=1
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_Quadrangulate_wireOnSurf_cullback
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --repr wireOnSurf --cullStyle BackUnlessDoubleSided --write testHdStBasicDrawing_quad_wireOnSurf_cullback.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_quad_wireOnSurf_cullback.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing_Quadrangulate
    ENV
        HD_ENABLE_FORCE_QUADRANGULATE=1
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_Quadrangulate_hull_cullfront
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --repr hull --cullStyle FrontUnlessDoubleSided --write testHdStBasicDrawing_quad_hull_cullfront.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_quad_hull_cullfront.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing_Quadrangulate
    ENV
        HD_ENABLE_FORCE_QUADRANGULATE=1
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_Quadrangulate_smoothHull_cullfront
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --repr smoothHull --cullStyle FrontUnlessDoubleSided --write testHdStBasicDrawing_quad_smoothHull_cullfront.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_quad_smoothHull_cullfront.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing_Quadrangulate
    ENV
        HD_ENABLE_FORCE_QUADRANGULATE=1
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_Quadrangulate_wire_cullfront
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --repr wire --cullStyle FrontUnlessDoubleSided --write testHdStBasicDrawing_quad_wire_cullfront.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_quad_wire_cullfront.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing_Quadrangulate
    ENV
        HD_ENABLE_FORCE_QUADRANGULATE=1
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBasicDrawing_Quadrangulate_wireOnSurf_cullfront
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --repr wireOnSurf --cullStyle FrontUnlessDoubleSided --write testHdStBasicDrawing_quad_wireOnSurf_cullfront.png"
    IMAGE_DIFF_COMPARE
        testHdStBasicDrawing_quad_wireOnSurf_cullfront.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBasicDrawing_Quadrangulate
    ENV
        HD_ENABLE_FORCE_QUADRANGULATE=1
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBufferAggregation
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBufferAggregation"
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBufferAggregation
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBufferArray
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBufferArray"
    EXPECTED_RETURN_CODE 0
    STDOUT_REDIRECT testHdStBufferArray-Run1-stdout.txt
    POST_COMMAND "diff -I drawingShader -I computeShader -I gpuMemoryUsed -I meshTopology -I basisCurvesTopology testHdStBufferArray-Run1-stdout.txt ${CMAKE_INSTALL_PREFIX}/tests/ctest/testHdStBufferArray/baseline/testHdStBufferArray-Run1-stdout.txt"
    TESTENV testHdStBufferArray
    ENV
        TF_DEBUG=HD_SAFE_MODE
        HD_ENABLE_DOUBLE_MATRIX=1
        HD_ENABLE_PACKED_NORMALS=0
)
pxr_register_test(testHdStBufferArrayInstancingDisabled
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBufferArray"
    EXPECTED_RETURN_CODE 0
    STDOUT_REDIRECT testHdStBufferArrayInstancingDisabled-Run1-stdout.txt
    POST_COMMAND "diff -I drawingShader -I computeShader -I gpuMemoryUsed -I meshTopology -I basisCurvesTopology testHdStBufferArrayInstancingDisabled-Run1-stdout.txt ${CMAKE_INSTALL_PREFIX}/tests/ctest/testHdStBufferArrayInstancingDisabled/baseline/testHdStBufferArrayInstancingDisabled-Run1-stdout.txt"
    TESTENV testHdStBufferArrayInstancingDisabled
    ENV
        HDST_ENABLE_RESOURCE_INSTANCING=0
        HD_ENABLE_DOUBLE_MATRIX=1
        HD_ENABLE_PACKED_NORMALS=0
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStBufferSourceDependency
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBufferSourceDependency"
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStBufferSourceDependency
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStClipPlanes1
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --clipPlane -1 0 0 -1.5 --write testHdStClipPlanes1.png"
    IMAGE_DIFF_COMPARE
        testHdStClipPlanes1.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStClipPlanes
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStClipPlanes2
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStBasicDrawing --offscreen --clipPlane 1 0 0 1.5 --clipPlane -1 0 0 1.5 --write testHdStClipPlanes2.png"
    IMAGE_DIFF_COMPARE
        testHdStClipPlanes2.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStClipPlanes
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStCodeGen_Mesh_Indirect
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStCodeGen --mesh | grep -v '// line'"
    EXPECTED_RETURN_CODE 0
    STDOUT_REDIRECT codegen_mesh_indirect.out
    POST_COMMAND "diff -I '\".*\.glslfx\"' codegen_mesh_indirect.out ${CMAKE_INSTALL_PREFIX}/tests/ctest/testHdStCodeGen/baseline/codegen_mesh_indirect.out"
    TESTENV testHdStCodeGen
    ENV
        TF_DEBUG=HD_SAFE_MODE
        HGIGL_ENABLE_BINDLESS_BUFFER=1
        HGIGL_ENABLE_BINDLESS_TEXTURE=1
        HGIGL_GLSL_VERSION=450
)
pxr_register_test(testHdStCodeGen_Mesh_Bindless_Indirect
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStCodeGen --mesh --bindless | grep -v '// line'"
    EXPECTED_RETURN_CODE 0
    STDOUT_REDIRECT codegen_mesh_bindless_indirect.out
    POST_COMMAND "diff -I '\".*\.glslfx\"' codegen_mesh_bindless_indirect.out ${CMAKE_INSTALL_PREFIX}/tests/ctest/testHdStCodeGen/baseline/codegen_mesh_bindless_indirect.out"
    TESTENV testHdStCodeGen
    ENV
        TF_DEBUG=HD_SAFE_MODE
        HGIGL_ENABLE_BINDLESS_BUFFER=1
        HGIGL_ENABLE_BINDLESS_TEXTURE=1
        HGIGL_GLSL_VERSION=450
)
pxr_register_test(testHdStCodeGen_Mesh_Bindless_Indirect_SmoothNormals
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStCodeGen --mesh --bindless --smoothNormals | grep -v '// line'"
    EXPECTED_RETURN_CODE 0
    STDOUT_REDIRECT codegen_mesh_bindless_indirect_smoothNormals.out
    POST_COMMAND "diff -I '\".*\.glslfx\"' codegen_mesh_bindless_indirect_smoothNormals.out ${CMAKE_INSTALL_PREFIX}/tests/ctest/testHdStCodeGen/baseline/codegen_mesh_bindless_indirect_smoothNormals.out"
    TESTENV testHdStCodeGen
    ENV
        TF_DEBUG=HD_SAFE_MODE
        HGIGL_ENABLE_BINDLESS_BUFFER=1
        HGIGL_ENABLE_BINDLESS_TEXTURE=1
        HGIGL_GLSL_VERSION=450
)
pxr_register_test(testHdStCodeGen_Mesh_Bindless_Indirect_DoubleSided
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStCodeGen --mesh --bindless --doubleSided | grep -v '// line'"
    EXPECTED_RETURN_CODE 0
    STDOUT_REDIRECT codegen_mesh_bindless_indirect_doubleSided.out
    POST_COMMAND "diff -I '\".*\.glslfx\"' codegen_mesh_bindless_indirect_doubleSided.out ${CMAKE_INSTALL_PREFIX}/tests/ctest/testHdStCodeGen/baseline/codegen_mesh_bindless_indirect_doubleSided.out"
    TESTENV testHdStCodeGen
    ENV
        TF_DEBUG=HD_SAFE_MODE
        HGIGL_ENABLE_BINDLESS_BUFFER=1
        HGIGL_ENABLE_BINDLESS_TEXTURE=1
        HGIGL_GLSL_VERSION=450
)
pxr_register_test(testHdStCodeGen_Mesh_Bindless_Indirect_FaceVarying
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStCodeGen --mesh --bindless --faceVarying | grep -v '// line'"
    EXPECTED_RETURN_CODE 0
    STDOUT_REDIRECT codegen_mesh_bindless_indirect_faceVarying.out
    POST_COMMAND "diff -I '\".*\.glslfx\"' codegen_mesh_bindless_indirect_faceVarying.out ${CMAKE_INSTALL_PREFIX}/tests/ctest/testHdStCodeGen/baseline/codegen_mesh_bindless_indirect_faceVarying.out"
    TESTENV testHdStCodeGen
    ENV
        TF_DEBUG=HD_SAFE_MODE
        HGIGL_ENABLE_BINDLESS_BUFFER=1
        HGIGL_ENABLE_BINDLESS_TEXTURE=1
        HGIGL_GLSL_VERSION=450
)
pxr_register_test(testHdStCodeGen_Mesh_Indirect_Instance
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStCodeGen --mesh --instance | grep -v '// line'"
    EXPECTED_RETURN_CODE 0
    STDOUT_REDIRECT codegen_mesh_indirect_instance.out
    POST_COMMAND "diff -I '\".*\.glslfx\"' codegen_mesh_indirect_instance.out ${CMAKE_INSTALL_PREFIX}/tests/ctest/testHdStCodeGen/baseline/codegen_mesh_indirect_instance.out"
    TESTENV testHdStCodeGen
    ENV
        TF_DEBUG=HD_SAFE_MODE
        HGIGL_ENABLE_BINDLESS_BUFFER=1
        HGIGL_ENABLE_BINDLESS_TEXTURE=1
        HGIGL_GLSL_VERSION=450
)
pxr_register_test(testHdStCodeGen_Mesh_Bindless_Indirect_Instance
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStCodeGen --mesh --bindless --instance | grep -v '// line'"
    EXPECTED_RETURN_CODE 0
    STDOUT_REDIRECT codegen_mesh_bindless_indirect_instance.out
    POST_COMMAND "diff -I '\".*\.glslfx\"' codegen_mesh_bindless_indirect_instance.out ${CMAKE_INSTALL_PREFIX}/tests/ctest/testHdStCodeGen/baseline/codegen_mesh_bindless_indirect_instance.out"
    TESTENV testHdStCodeGen
    ENV
        TF_DEBUG=HD_SAFE_MODE
        HGIGL_ENABLE_BINDLESS_BUFFER=1
        HGIGL_ENABLE_BINDLESS_TEXTURE=1
        HGIGL_GLSL_VERSION=450
)
pxr_register_test(testHdStCodeGen_Mesh_EdgeOnly
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStCodeGen --mesh --edgeOnly | grep -v '// line'"
    EXPECTED_RETURN_CODE 0
    STDOUT_REDIRECT codegen_mesh_edgeonly.out
    POST_COMMAND "diff -I '\".*\.glslfx\"' codegen_mesh_edgeonly.out ${CMAKE_INSTALL_PREFIX}/tests/ctest/testHdStCodeGen/baseline/codegen_mesh_edgeonly.out"
    TESTENV testHdStCodeGen
    ENV
        TF_DEBUG=HD_SAFE_MODE
        HGIGL_ENABLE_BINDLESS_BUFFER=1
        HGIGL_ENABLE_BINDLESS_TEXTURE=1
        HGIGL_GLSL_VERSION=450
)
pxr_register_test(testHdStCodeGen_Mesh_EdgeOnly_BlendWireframe
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStCodeGen --mesh --edgeOnly --blendWireframe | grep -v '// line'"
    EXPECTED_RETURN_CODE 0
    STDOUT_REDIRECT codegen_mesh_edgeonly_blendwireframe.out
    POST_COMMAND "diff -I '\".*\.glslfx\"' codegen_mesh_edgeonly_blendwireframe.out ${CMAKE_INSTALL_PREFIX}/tests/ctest/testHdStCodeGen/baseline/codegen_mesh_edgeonly_blendwireframe.out"
    TESTENV testHdStCodeGen
    ENV
        TF_DEBUG=HD_SAFE_MODE
        HGIGL_ENABLE_BINDLESS_BUFFER=1
        HGIGL_ENABLE_BINDLESS_TEXTURE=1
        HGIGL_GLSL_VERSION=450
)
pxr_register_test(testHdStCodeGen_Curves_Bindless
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStCodeGen --curves --bindless | grep -v '// line'"
    EXPECTED_RETURN_CODE 0
    STDOUT_REDIRECT codegen_curves_bindless.out
    POST_COMMAND "diff -I '\".*\.glslfx\"' codegen_curves_bindless.out ${CMAKE_INSTALL_PREFIX}/tests/ctest/testHdStCodeGen/baseline/codegen_curves_bindless.out"
    TESTENV testHdStCodeGen
    ENV
        TF_DEBUG=HD_SAFE_MODE
        HGIGL_ENABLE_BINDLESS_BUFFER=1
        HGIGL_ENABLE_BINDLESS_TEXTURE=1
        HGIGL_GLSL_VERSION=450
)
pxr_register_test(testHdStCodeGen_Curves_Indirect
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStCodeGen --curves | grep -v '// line'"
    EXPECTED_RETURN_CODE 0
    STDOUT_REDIRECT codegen_curves_indirect.out
    POST_COMMAND "diff -I '\".*\.glslfx\"' codegen_curves_indirect.out ${CMAKE_INSTALL_PREFIX}/tests/ctest/testHdStCodeGen/baseline/codegen_curves_indirect.out"
    TESTENV testHdStCodeGen
    ENV
        TF_DEBUG=HD_SAFE_MODE
        HGIGL_ENABLE_BINDLESS_BUFFER=1
        HGIGL_ENABLE_BINDLESS_TEXTURE=1
        HGIGL_GLSL_VERSION=450
)
pxr_register_test(testHdStCodeGen_Points_Bindless
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStCodeGen --points --bindless | grep -v '// line'"
    EXPECTED_RETURN_CODE 0
    STDOUT_REDIRECT codegen_points_bindless.out
    POST_COMMAND "diff -I '\".*\.glslfx\"' codegen_points_bindless.out ${CMAKE_INSTALL_PREFIX}/tests/ctest/testHdStCodeGen/baseline/codegen_points_bindless.out"
    TESTENV testHdStCodeGen
    ENV
        TF_DEBUG=HD_SAFE_MODE
        HGIGL_ENABLE_BINDLESS_BUFFER=1
        HGIGL_ENABLE_BINDLESS_TEXTURE=1
        HGIGL_GLSL_VERSION=450
)
pxr_register_test(testHdStCodeGen_Points_Indirect
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStCodeGen --points | grep -v '// line'"
    EXPECTED_RETURN_CODE 0
    STDOUT_REDIRECT codegen_points_indirect.out
    POST_COMMAND "diff -I '\".*\.glslfx\"' codegen_points_indirect.out ${CMAKE_INSTALL_PREFIX}/tests/ctest/testHdStCodeGen/baseline/codegen_points_indirect.out"
    TESTENV testHdStCodeGen
    ENV
        TF_DEBUG=HD_SAFE_MODE
        HGIGL_ENABLE_BINDLESS_BUFFER=1
        HGIGL_ENABLE_BINDLESS_TEXTURE=1
        HGIGL_GLSL_VERSION=450
)
pxr_register_test(testHdStCurveDrawing_lv0_refined
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStCurveDrawing --offscreen --repr refined --refineLevel 0 --write testHdStCurveDrawing_lv0_refined.png"
    EXPECTED_RETURN_CODE 0
    IMAGE_DIFF_COMPARE
        testHdStCurveDrawing_lv0_refined.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    TESTENV testHdStCurveDrawing
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStCurveDrawing_lv1_refined
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStCurveDrawing --offscreen --repr refined --refineLevel 1 --write testHdStCurveDrawing_lv1_refined.png"
    EXPECTED_RETURN_CODE 0
    IMAGE_DIFF_COMPARE
        testHdStCurveDrawing_lv1_refined.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    TESTENV testHdStCurveDrawing
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStCurvePrimvarInterpolation_lv0_refined
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStCurvePrimvarInterpolation --offscreen --repr refined --refineLevel 0 --write testHdStCurvePrimvarInterpolation_lv0_refined.png"
    EXPECTED_RETURN_CODE 0
    IMAGE_DIFF_COMPARE
        testHdStCurvePrimvarInterpolation_lv0_refined.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    TESTENV testHdStCurvePrimvarInterpolation
    ENV
        TF_DEBUG=HD_DISABLE_MULTITHREADED_RPRIM_SYNC
)
pxr_register_test(testHdStCurvePrimvarInterpolation_lv1_refined
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStCurvePrimvarInterpolation --offscreen --repr refined --refineLevel 1 --write testHdStCurvePrimvarInterpolation_lv1_refined.png"
    EXPECTED_RETURN_CODE 0
    IMAGE_DIFF_COMPARE
        testHdStCurvePrimvarInterpolation_lv1_refined.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    TESTENV testHdStCurvePrimvarInterpolation
    ENV
        TF_DEBUG=HD_DISABLE_MULTITHREADED_RPRIM_SYNC
)
pxr_register_test(testHdStCurvePrimvarInterpolation_lv3_refined
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStCurvePrimvarInterpolation --offscreen --repr refined --refineLevel 3 --write testHdStCurvePrimvarInterpolation_lv3_refined.png"
    EXPECTED_RETURN_CODE 0
    IMAGE_DIFF_COMPARE
        testHdStCurvePrimvarInterpolation_lv3_refined.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    TESTENV testHdStCurvePrimvarInterpolation
    ENV
        TF_DEBUG=HD_DISABLE_MULTITHREADED_RPRIM_SYNC
)
pxr_register_test(testHdStCurvesPrimvars
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStCurvesPrimvars"
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStCurvesPrimvars
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStDisplayStyle
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStDisplayStyle --offscreen --write testHdStDisplayStyle.png"
    EXPECTED_RETURN_CODE 0
    IMAGE_DIFF_COMPARE
        testHdStDisplayStyle.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    TESTENV testHdStDisplayStyle
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStDisplayStyle_refined
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStDisplayStyle --offscreen --refineLevel 2 --write testHdStDisplayStyle_refined.png"
    EXPECTED_RETURN_CODE 0
    IMAGE_DIFF_COMPARE
        testHdStDisplayStyle_refined.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    TESTENV testHdStDisplayStyle
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStDrawBatching
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStDrawBatching"
    EXPECTED_RETURN_CODE 0
    STDOUT_REDIRECT testHdStDrawBatching-Run1-stdout.txt
    POST_COMMAND "diff -I computeShader -I drawingShader -I gpuMemoryUsed testHdStDrawBatching-Run1-stdout.txt ${CMAKE_INSTALL_PREFIX}/tests/ctest/testHdStDrawBatching/baseline/testHdStDrawBatching-Run1-stdout.txt"
    TESTENV testHdStDrawBatching
    ENV
        TF_DEBUG=HD_SAFE_MODE
        HD_ENABLE_DOUBLE_MATRIX=1
        HD_ENABLE_GPU_COUNT_VISIBLE_INSTANCES=1
        HD_ENABLE_PACKED_NORMALS=0
)
pxr_register_test(testHdStDrawItemIntegrity
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStDrawItemIntegrity --offscreen"
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStDrawItemIntegrity
)
pxr_register_test(testHdStDrawItemsCache
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStDrawItemsCache --offscreen"
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStDrawItemsCache
    ENV
        HDST_ENABLE_DRAW_ITEMS_CACHE=1
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStDynamicUvTexture
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStDynamicUvTexture --offscreen"
    EXPECTED_RETURN_CODE 0
    IMAGE_DIFF_COMPARE
        outTextureDarkBlue.png
        outTextureLightBlue.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    TESTENV testHdStDynamicUvTexture
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStFaceCulling_None
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStFaceCulling --offscreen --repr wireOnSurf --cullStyle Nothing --write testHdStFaceCulling_cull_none.png"
    EXPECTED_RETURN_CODE 0
    IMAGE_DIFF_COMPARE
        testHdStFaceCulling_cull_none.png
    FAIL 1
    FAIL_PERCENT 1
    TESTENV testHdStFaceCulling
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStFaceCulling_Back
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStFaceCulling --offscreen --repr wireOnSurf --cullStyle Back --write testHdStFaceCulling_cull_back.png"
    EXPECTED_RETURN_CODE 0
    IMAGE_DIFF_COMPARE
        testHdStFaceCulling_cull_back.png
    FAIL 1
    FAIL_PERCENT 1
    TESTENV testHdStFaceCulling
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStFaceCulling_Front
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStFaceCulling --offscreen --repr wireOnSurf --cullStyle Front --write testHdStFaceCulling_cull_front.png"
    EXPECTED_RETURN_CODE 0
    IMAGE_DIFF_COMPARE
        testHdStFaceCulling_cull_front.png
    FAIL 1
    FAIL_PERCENT 1
    TESTENV testHdStFaceCulling
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStFaceCulling_BackUnlessDoubleSided
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStFaceCulling --offscreen --repr wireOnSurf --cullStyle BackUnlessDoubleSided --write testHdStFaceCulling_cull_backUnlessDS.png"
    EXPECTED_RETURN_CODE 0
    IMAGE_DIFF_COMPARE
        testHdStFaceCulling_cull_backUnlessDS.png
    FAIL 1
    FAIL_PERCENT 1
    TESTENV testHdStFaceCulling
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStFaceCulling_FrontUnlessDoubleSided
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStFaceCulling --offscreen --repr wireOnSurf --cullStyle FrontUnlessDoubleSided --write testHdStFaceCulling_cull_frontUnlessDS.png"
    EXPECTED_RETURN_CODE 0
    IMAGE_DIFF_COMPARE
        testHdStFaceCulling_cull_frontUnlessDS.png
    FAIL 1
    FAIL_PERCENT 1
    TESTENV testHdStFaceCulling
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStFrustumCullingCPU
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStFrustumCulling --offscreen"
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStFaceCulling
    ENV
        HD_ENABLE_GPU_FRUSTUM_CULLING=0
)
pxr_register_test(testHdStFrustumCullingGPU
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStFrustumCulling --offscreen"
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStFrustumCulling
    ENV
        HD_ENABLE_GPU_FRUSTUM_CULLING=1
        HD_ENABLE_GPU_COUNT_VISIBLE_INSTANCES=1
)
pxr_register_test(testHdStFrustumCullingGPUTinyPrims
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStFrustumCulling --offscreen --tinyprim"
    EXPECTED_RETURN_CODE 0
    ENV
        HD_ENABLE_GPU_COUNT_VISIBLE_INSTANCES=1
        HD_ENABLE_GPU_TINY_PRIM_CULLING=1
        HD_ENABLE_GPU_FRUSTUM_CULLING=1
)
pxr_register_test(testHdStFrustumCullingInstanceGPU
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStFrustumCulling --offscreen --instance"
    EXPECTED_RETURN_CODE 0
    ENV
        HD_ENABLE_GPU_COUNT_VISIBLE_INSTANCES=1
        HD_ENABLE_GPU_FRUSTUM_CULLING=1
)
pxr_register_test(testHdStGL
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStGL"
    EXPECTED_RETURN_CODE 0
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStGLSL_instanceTransform0
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStGLSL testHarness.glslfx instanceTransform0"
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStGLSL
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStGLSL_instanceTransform1
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStGLSL testHarness.glslfx instanceTransform1"
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStGLSL
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStGLSL_instanceTransform2
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStGLSL testHarness.glslfx instanceTransform2"
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStGLSL
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStGLSL_instanceTransform3
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStGLSL testHarness.glslfx instanceTransform3"
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStGLSL
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStGLSL_determinant
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStGLSL testHarness.glslfx determinant"
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStGLSL
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStHWFaceCulling
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStHWFaceCulling"
    EXPECTED_RETURN_CODE 0
    STDOUT_REDIRECT testHdStHWFaceCulling-Run1-stdout.txt
    POST_COMMAND "diff -I computeShader -I drawingShader -I gpuMemoryUsed testHdStHWFaceCulling-Run1-stdout.txt ${CMAKE_INSTALL_PREFIX}/tests/ctest/testHdStHWFaceCulling/baseline/testHdStHWFaceCulling-Run1-stdout.txt"
    TESTENV testHdStHWFaceCulling
)
pxr_register_test(testHdStIndirectDrawBatchCodeGen
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStIndirectDrawBatchCodeGen"
    EXPECTED_RETURN_CODE 0
    ENV
        HD_ENABLE_GPU_FRUSTUM_CULLING=false
)
pxr_register_test(testHdStIndirectDrawBatchCulling
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStIndirectDrawBatchCulling"
    EXPECTED_RETURN_CODE 0
    ENV
        HD_ENABLE_GPU_TINY_PRIM_CULLING=true
        HD_ENABLE_GPU_FRUSTUM_CULLING=true
)
pxr_register_test(testHdStInitRepr
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStInitRepr --offscreen --outputFilePrefix testHdStInitRepr"
    EXPECTED_RETURN_CODE 0
    IMAGE_DIFF_COMPARE
        testHdStInitRepr_0.png
        testHdStInitRepr_1.png
        testHdStInitRepr_2.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    TESTENV testHdStInitRepr
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStInstancing
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStInstancing --offscreen --write testHdStInstancing.png"
    EXPECTED_RETURN_CODE 0
    IMAGE_DIFF_COMPARE
        testHdStInstancing.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    TESTENV testHdStInstancing
    ENV
        HGIGL_ENABLE_BINDLESS_BUFFER=1
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStInstancing_Refined
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStInstancing --offscreen --refineLevel 2 --repr refined --write testHdStInstancing_refined.png"
    EXPECTED_RETURN_CODE 0
    IMAGE_DIFF_COMPARE
        testHdStInstancing_refined.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    TESTENV testHdStInstancing
    ENV
        HGIGL_ENABLE_BINDLESS_BUFFER=1
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStInstancing_Wire
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStInstancing --offscreen --repr wire --write testHdStInstancing_wire.png"
    EXPECTED_RETURN_CODE 0
    IMAGE_DIFF_COMPARE
        testHdStInstancing_wire.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    TESTENV testHdStInstancing
    ENV
        HGIGL_ENABLE_BINDLESS_BUFFER=1
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStInstancing_NoPrimvars
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStInstancing --offscreen --noprimvars"
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStInstancing
    ENV
        HGIGL_ENABLE_BINDLESS_BUFFER=1
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStInstancing_Div4_Level4
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStInstancing --offscreen --div 5 --level 2 --write testHdStInstancing_div5_level2.png"
    EXPECTED_RETURN_CODE 0
    IMAGE_DIFF_COMPARE
        testHdStInstancing_div5_level2.png
    FAIL 1
    FAIL_PERCENT 1
    TESTENV testHdStInstancing
    ENV
        HGIGL_ENABLE_BINDLESS_BUFFER=1
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStInstancing_Div5_Level2
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStInstancing --offscreen --div 4 --level 4 --repr refined --refineLevel 2 --write testHdStInstancing_div4_level4_refined.png"
    EXPECTED_RETURN_CODE 0
    IMAGE_DIFF_COMPARE
        testHdStInstancing_div4_level4_refined.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    TESTENV testHdStInstancing
    ENV
        HGIGL_ENABLE_BINDLESS_BUFFER=1
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStInstancing_Div3_Level3
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStInstancing --offscreen --div 3 --level 3 --rootTransform --write testHdStInstancing_div3_level3_rootTransform.png"
    EXPECTED_RETURN_CODE 0
    IMAGE_DIFF_COMPARE
        testHdStInstancing_div3_level3_rootTransform.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    TESTENV testHdStInstancing
    ENV
        HGIGL_ENABLE_BINDLESS_BUFFER=1
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStInstancingUnbalanced
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStInstancingUnbalanced --offscreen --write testHdStInstancingUnbalanced.png"
    EXPECTED_RETURN_CODE 0
    IMAGE_DIFF_COMPARE
        testHdStInstancingUnbalanced.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    TESTENV testHdStInstancingUnbalanced
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStInstancingUnbalancedNoBindless
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStInstancingUnbalanced --offscreen --write testHdStInstancingUnbalancedNoBindless.png"
    EXPECTED_RETURN_CODE 0
    IMAGE_DIFF_COMPARE
        testHdStInstancingUnbalancedNoBindless.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    TESTENV testHdStInstancingUnbalancedNoBindless
    ENV
        HGIGL_ENABLE_BINDLESS_BUFFER=0
        TF_DEBUG=HD_SAFE_MODE
)
if (${PXR_ENABLE_MATERIALX_SUPPORT})
pxr_register_test(testHdStMaterialXShaderGen_SSdefault
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStMaterialXShaderGen --filename standard_surface_default.mtlx"
    STDOUT_REDIRECT shadergen_SSdefault.out
    EXPECTED_RETURN_CODE 0
    POST_COMMAND "diff -B -b shadergen_SSdefault.out ${CMAKE_INSTALL_PREFIX}/tests/ctest/testHdStMaterialXShaderGen/baseline/shadergen_SSdefault.out"
    TESTENV testHdStMaterialXShaderGen
)
pxr_register_test(testHdStMaterialXShaderGen_UsdPSdefault
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStMaterialXShaderGen --filename usd_preview_surface_default.mtlx"
    STDOUT_REDIRECT shadergen_UsdPSdefault.out
    EXPECTED_RETURN_CODE 0
    POST_COMMAND "diff -B -b shadergen_UsdPSdefault.out ${CMAKE_INSTALL_PREFIX}/tests/ctest/testHdStMaterialXShaderGen/baseline/shadergen_UsdPSdefault.out"
    TESTENV testHdStMaterialXShaderGen
)
pxr_register_test(testHdStMaterialXShaderGen_SSmarble
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStMaterialXShaderGen --filename standard_surface_marble_solid.mtlx"
    STDOUT_REDIRECT shadergen_SSmarble.out
    EXPECTED_RETURN_CODE 0
    POST_COMMAND "diff -B -b shadergen_SSmarble.out ${CMAKE_INSTALL_PREFIX}/tests/ctest/testHdStMaterialXShaderGen/baseline/shadergen_SSmarble.out"
    TESTENV testHdStMaterialXShaderGen
)
pxr_register_test(testHdStMaterialXShaderGen_SScopper
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStMaterialXShaderGen --filename standard_surface_copper.mtlx"
    STDOUT_REDIRECT shadergen_SScopper.out
    EXPECTED_RETURN_CODE 0
    POST_COMMAND "diff -B -b shadergen_SScopper.out ${CMAKE_INSTALL_PREFIX}/tests/ctest/testHdStMaterialXShaderGen/baseline/shadergen_SScopper.out"
    TESTENV testHdStMaterialXShaderGen
)
pxr_register_test(testHdStMaterialXShaderGen_UsdPStextured
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStMaterialXShaderGen --filename usd_preview_surface_brass_tiled.mtlx --textureMap image_color_file:diffuseColor --textureMap image_roughness_file:roughness"
    STDOUT_REDIRECT shadergen_UsdPStextured.out
    EXPECTED_RETURN_CODE 0
    POST_COMMAND "diff -B -b shadergen_UsdPStextured.out ${CMAKE_INSTALL_PREFIX}/tests/ctest/testHdStMaterialXShaderGen/baseline/shadergen_UsdPStextured.out"
    TESTENV testHdStMaterialXShaderGen
)
pxr_register_test(testHdStMaterialXShaderGen_SStextured
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStMaterialXShaderGen --filename standard_surface_brass_tiled.mtlx --textureMap image_color_file:diffuseColor --textureMap image_roughness_file:roughness --primvarMap uv:vector2"
    STDOUT_REDIRECT shadergen_SStextured.out
    EXPECTED_RETURN_CODE 0
    POST_COMMAND "diff -B -b shadergen_SStextured.out ${CMAKE_INSTALL_PREFIX}/tests/ctest/testHdStMaterialXShaderGen/baseline/shadergen_SStextured.out"
    TESTENV testHdStMaterialXShaderGen
)
pxr_register_test(testHdStMaterialXShaderGen_UsdPSglass
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStMaterialXShaderGen --filename usd_preview_surface_glass.mtlx --materialTag translucent"
    STDOUT_REDIRECT shadergen_UsdPSglass.out
    EXPECTED_RETURN_CODE 0
    POST_COMMAND "diff -B -b shadergen_UsdPSglass.out ${CMAKE_INSTALL_PREFIX}/tests/ctest/testHdStMaterialXShaderGen/baseline/shadergen_UsdPSglass.out"
    TESTENV testHdStMaterialXShaderGen
)
pxr_register_test(testHdStMaterialXShaderGen8_SStextured_bindless
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStMaterialXShaderGen --filename standard_surface_brass_tiled.mtlx --textureMap image_color_file:diffuseColor --textureMap image_roughness_file:roughness --primvarMap uv:vector2 --bindless"
    STDOUT_REDIRECT shadergen_SStextured_bindless.out
    EXPECTED_RETURN_CODE 0
    POST_COMMAND "diff -B -b shadergen_SStextured_bindless.out ${CMAKE_INSTALL_PREFIX}/tests/ctest/testHdStMaterialXShaderGen/baseline/shadergen_SStextured_bindless.out"
    TESTENV testHdStMaterialXShaderGen
)
pxr_register_test(testHdStMaterialXShaderGen_texcoord
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStMaterialXShaderGen --filename texcoord.mtlx --primvarMap st:vector2"
    STDOUT_REDIRECT shadergen_texcoord.out
    EXPECTED_RETURN_CODE 0
    POST_COMMAND "diff -B -b shadergen_texcoord.out ${CMAKE_INSTALL_PREFIX}/tests/ctest/testHdStMaterialXShaderGen/baseline/shadergen_texcoord.out"
    TESTENV testHdStMaterialXShaderGen
)
endif()
pxr_register_test(testHdStMeshTopology
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStMeshTopology"
    EXPECTED_RETURN_CODE 0
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStMultipleFvarTopologies
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStMultipleFvarTopologies --offscreen --repr refinedWireOnSurf --refineLevel 1"
    EXPECTED_RETURN_CODE 0
    IMAGE_DIFF_COMPARE
        testHdStMultipleFvarTopologies_0.png
        testHdStMultipleFvarTopologies_1.png
        testHdStMultipleFvarTopologies_2.png
        testHdStMultipleFvarTopologies_3.png
        testHdStMultipleFvarTopologies_4.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    TESTENV testHdStMultipleFvarTopologies
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStPrimGather
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStPrimGather  --offscreen"
    EXPECTED_RETURN_CODE 0
    IMAGE_DIFF_COMPARE
        testHdStNestedDelegate_.png
        testHdStNestedDelegate_a.png
        testHdStNestedDelegate_i.png
        testHdStNestedDelegate_i_a.png
        testHdStNestedDelegate_i_j.png
        testHdStNestedDelegate_i_j_a.png
        testHdStNestedDelegate_i_j_k.png
        testHdStNestedDelegate_i_j_k_a.png
        testHdStNestedDelegate_i_j_k_l.png
        testHdStNestedDelegate_i_j_k_z.png
        testHdStNestedDelegate_i_j_z.png
        testHdStNestedDelegate_i_z.png
        testHdStNestedDelegate_z.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    TESTENV testHdStPrimGather
)
pxr_register_test(testHdStPrimitiveParam
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStPrimitiveParam"
    EXPECTED_RETURN_CODE 0
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStPrimvars
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStPrimvars"
    EXPECTED_RETURN_CODE 0
    STDOUT_REDIRECT testHdStPrimvars-Run1-stdout.txt
    POST_COMMAND "diff -I computeShader -I drawingShader -I gpuMemoryUsed testHdStPrimvars-Run1-stdout.txt ${CMAKE_INSTALL_PREFIX}/tests/ctest/testHdStPrimvars/baseline/testHdStPrimvars-Run1-stdout.txt"
    TESTENV testHdStPrimvars
    ENV
        HDST_ENABLE_MATERIAL_PRIMVAR_FILTERING=0
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStQuadrangulation
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStQuadrangulation"
    EXPECTED_RETURN_CODE 0
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStRenderPass
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStRenderPass"
    EXPECTED_RETURN_CODE 0
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStSamplerObjectRegistry
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStSamplerObjectRegistry --offscreen"
    EXPECTED_RETURN_CODE 0
    IMAGE_DIFF_COMPARE
        outSampler1.png
        outSampler2.png
        outHiResSampler.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
)
pxr_register_test(testHdStSubdivisionOsd3
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStSubdivision"
    EXPECTED_RETURN_CODE 0
    STDOUT_REDIRECT testHdStSubdivisionOsd3-Run1-stdout.txt
    DIFF_COMPARE testHdStSubdivisionOsd3-Run1-stdout.txt
    TESTENV testHdStSubdivisionOsd3
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStTextureHandleRegistry
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStTextureHandleRegistry --offscreen"
    EXPECTED_RETURN_CODE 0
    IMAGE_DIFF_COMPARE
        outTextureBasic.png
        outTextureSampler1LowRes.png
        outTextureSampler1.png
        outTextureSampler2.png
        outTextureBeforeFileChange.png
        outTextureAfterFileChange.png
        outTextureAfterReload.png
        outTextureWithLowGlobalMemoryRequest.png
        outTextureWithHighGlobalMemoryRequest.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
)
pxr_register_test(testHdStTextureObjectRegistry
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStTextureObjectRegistry --offscreen"
    EXPECTED_RETURN_CODE 0
    IMAGE_DIFF_COMPARE
        outTexture1FullRes.png
        outTexture1LowRes.png
        outTexture1Reloaded.png
        outTexture2.png
        outGrayscaleTexture.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
)
pxr_register_test(testHdStTransition0
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStTransition --offscreen --outputFilePrefix testHdStTransition0 --repr hull -c draw,animate=/cube1,repr=smoothHull,draw,repr=hull,draw,refineLevel=1,repr=refined,draw,refineLevel=2,draw"
    EXPECTED_RETURN_CODE 0
    IMAGE_DIFF_COMPARE
        testHdStTransition0_0.png
        testHdStTransition0_1.png
        testHdStTransition0_2.png
        testHdStTransition0_3.png
        testHdStTransition0_4.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    TESTENV testHdStTransition
    ENV
        HD_ENABLE_FORCE_QUADRANGULATE=1
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStTransition1
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStTransition --offscreen --outputFilePrefix testHdStTransition1 --repr smoothHull -c draw,animate=/cube1,animate=/grid1,animate=/curve2,animate=/points3,repr=hull,draw"
    EXPECTED_RETURN_CODE 0
    IMAGE_DIFF_COMPARE
        testHdStTransition1_0.png
        testHdStTransition1_1.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    TESTENV testHdStTransition
    ENV
        HD_ENABLE_FORCE_QUADRANGULATE=1
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStTransition2
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStTransition --offscreen --outputFilePrefix testHdStTransition2 --repr smoothHull -c draw,clipPlane=1:0:0:1,draw,clipPlane=-1:0:0:1,draw"
    EXPECTED_RETURN_CODE 0
    IMAGE_DIFF_COMPARE
        testHdStTransition2_0.png
        testHdStTransition2_1.png
        testHdStTransition2_2.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    TESTENV testHdStTransition
    ENV
        HD_ENABLE_FORCE_QUADRANGULATE=1
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStTransitionRefinedCurves
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStTransition --offscreen --outputFilePrefix testHdStTransitionRefinedCurves0 --repr hull -c draw,repr=smoothHull,draw"
    EXPECTED_RETURN_CODE 0
    IMAGE_DIFF_COMPARE
        testHdStTransitionRefinedCurves0_0.png
        testHdStTransitionRefinedCurves0_1.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    TESTENV testHdStTransitionRefinedCurves
    ENV
        HD_ENABLE_REFINED_CURVES=1
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStSubResourceRegistry
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStSubResourceRegistry"
    EXPECTED_RETURN_CODE 0
    TESTENV testHdStSubResourceRegistry
    ENV
        TF_DEBUG=HD_SAFE_MODE
)

# Certain tests use plugins that require shared libraries.
if (TARGET shared_libs)

if (PXR_ENABLE_PTEX_SUPPORT)
pxr_build_test(testHdStPtex
    LIBRARIES
        hdSt
        hd
        sdf
        tf
        gf
    CPPFILES
        testenv/testHdStPtex.cpp
)
pxr_install_test_dir(
    SRC testenv/testHdStPtex
    DEST testHdStPtex
)
pxr_install_test_dir(
    SRC testenv/testHdStPtexTetrahedron
    DEST testHdStPtexTetrahedron
)
pxr_register_test(testHdStPtex
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStPtex --ptex test.ptx --offscreen --write testHdStPtex.png"
    EXPECTED_RETURN_CODE 0
    IMAGE_DIFF_COMPARE
        testHdStPtex.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    TESTENV testHdStPtex
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStPtex_Refined
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStPtex --ptex test.ptx --offscreen --refineLevel 2 --repr refined --write testHdStPtex_refined.png"
    EXPECTED_RETURN_CODE 0
    IMAGE_DIFF_COMPARE
        testHdStPtex_refined.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    TESTENV testHdStPtex
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStPtexBindless
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStPtex --ptex test.ptx --offscreen --write testHdStPtex.png"
    EXPECTED_RETURN_CODE 0
    IMAGE_DIFF_COMPARE
        testHdStPtex.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    TESTENV testHdStPtex
    ENV
        HGIGL_ENABLE_BINDLESS_BUFFER=true
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStPtexBindless_Refined
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStPtex --ptex test.ptx --offscreen --refineLevel 2 --repr refined --write testHdStPtex_refined.png"
    EXPECTED_RETURN_CODE 0
    IMAGE_DIFF_COMPARE
        testHdStPtex_refined.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    TESTENV testHdStPtex
    ENV
        HGIGL_ENABLE_BINDLESS_BUFFER=true
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStPtexTetrahedron
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStPtex --ptex tetrahedron.ptx --offscreen --write testHdStPtexTetrahedron.png"
    EXPECTED_RETURN_CODE 0
    IMAGE_DIFF_COMPARE
        testHdStPtexTetrahedron.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    TESTENV testHdStPtexTetrahedron
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStPtexTetrahedron_Loop
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStPtex --loop --ptex tetrahedron.ptx --offscreen --write testHdStPtexTetrahedron_loop.png"
    EXPECTED_RETURN_CODE 0
    IMAGE_DIFF_COMPARE
        testHdStPtexTetrahedron_loop.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    TESTENV testHdStPtexTetrahedron
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStPtexTetrahedron_ForceQuads
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStPtex --ptex tetrahedron_force_quads.ptx --offscreen --write testHdStPtexTetrahedron_force_quads.png"
    EXPECTED_RETURN_CODE 0
    IMAGE_DIFF_COMPARE
        testHdStPtexTetrahedron_force_quads.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    TESTENV testHdStPtexTetrahedron
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStPtexTetrahedron_Quads
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStPtex --ptex tetrahedron_quads.ptx --offscreen --write testHdStPtexTetrahedron_quads.png"
    EXPECTED_RETURN_CODE 0
    IMAGE_DIFF_COMPARE
        testHdStPtexTetrahedron_quads.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    TESTENV testHdStPtexTetrahedron
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStPtexTetrahedron_Quads_ForceTris
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStPtex --ptex tetrahedron_quads_force_tris.ptx --offscreen --write testHdStPtexTetrahedron_quads_force_tris.png"
    EXPECTED_RETURN_CODE 0
    IMAGE_DIFF_COMPARE
        testHdStPtexTetrahedron_quads_force_tris.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    TESTENV testHdStPtexTetrahedron
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStPtexTetrahedron_Refined
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStPtex --ptex tetrahedron.ptx --offscreen --refineLevel 2 --repr refined --write testHdStPtexTetrahedron_refined.png"
    EXPECTED_RETURN_CODE 0
    IMAGE_DIFF_COMPARE
        testHdStPtexTetrahedron_refined.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    TESTENV testHdStPtexTetrahedron
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStPtexTetrahedron_Refined_Loop
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStPtex --loop --ptex tetrahedron.ptx --offscreen --refineLevel 2 --repr refined --write testHdStPtexTetrahedron_refined_loop.png"
    EXPECTED_RETURN_CODE 0
    IMAGE_DIFF_COMPARE
        testHdStPtexTetrahedron_refined_loop.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    TESTENV testHdStPtexTetrahedron
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStPtexTetrahedron_Refined_ForceQuads
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStPtex --ptex tetrahedron_force_quads.ptx --offscreen --refineLevel 2 --repr refined --write testHdStPtexTetrahedron_force_quads_refined.png"
    EXPECTED_RETURN_CODE 0
    IMAGE_DIFF_COMPARE
        testHdStPtexTetrahedron_force_quads_refined.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    TESTENV testHdStPtexTetrahedron
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStPtexTetrahedron_Refined_Quads
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStPtex --ptex tetrahedron_quads.ptx --offscreen --refineLevel 2 --repr refined --write testHdStPtexTetrahedron_quads_refined.png"
    EXPECTED_RETURN_CODE 0
    IMAGE_DIFF_COMPARE
        testHdStPtexTetrahedron_quads_refined.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    TESTENV testHdStPtexTetrahedron
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdStPtexTetrahedron_Refined_Quads_ForceTris
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStPtex --ptex tetrahedron_quads_force_tris.ptx --offscreen --refineLevel 2 --repr refined --write testHdStPtexTetrahedron_quads_force_tris_refined.png"
    EXPECTED_RETURN_CODE 0
    IMAGE_DIFF_COMPARE
        testHdStPtexTetrahedron_quads_force_tris_refined.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    TESTENV testHdStPtexTetrahedron
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
endif() # PXR_ENABLE_PTEX_SUPPORT

pxr_build_test(testHdStShaders
    LIBRARIES
        hdSt
        hd
        sdr
        tf
        gf
    CPPFILES
        testenv/testHdStShaders.cpp
)
pxr_install_test_dir(
    SRC testenv/testHdStShaders
    DEST testHdStShaders
)
pxr_register_test(testHdStShaders
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdStShaders --offscreen --outputFilePrefix testHdStShaders"
    EXPECTED_RETURN_CODE 0
    IMAGE_DIFF_COMPARE
        testHdStShaders_0.png
        testHdStShaders_1.png
        testHdStShaders_2.png
        testHdStShaders_3.png
        testHdStShaders_4.png
        testHdStShaders_5.png
        testHdStShaders_6.png
        testHdStShaders_7.png
        testHdStShaders_8.png
        testHdStShaders_9.png
        testHdStShaders_10.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    TESTENV testHdStShaders
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
endif() # TARGET shared_libs
