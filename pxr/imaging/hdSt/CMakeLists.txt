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
pxr_build_test(testHdStTransition
    LIBRARIES
        hdSt
        hd
        tf
        gf
    CPPFILES
        testenv/testHdStTransition.cpp
)
endif()

if (${PXR_HEADLESS_TEST_MODE})
    message(STATUS "Skipping ${PXR_PACKAGE} tests because PXR_HEADLESS_TEST_MODE is ON")
    return()
endif()

if (APPLE)
    message(STATUS "Skipping ${PXR_PACKAGE} tests because they are currently unsupported on macOS")
    return()
endif()

if (WIN32)
    message(STATUS "Skipping ${PXR_PACKAGE} tests because they are currently unsupported on Windows")
    return()
endif()

#
# Install a selection of the test data
#

pxr_install_test_dir(
    SRC testenv/testHdStBasicDrawing
    DEST testHdStBasicDrawing
)

pxr_install_test_dir(
    SRC testenv/testHdStBasicDrawing_Adaptive
    DEST testHdStBasicDrawing_Adaptive
)

pxr_install_test_dir(
    SRC testenv/testHdStBasicDrawing_Quadrangulate
    DEST testHdStBasicDrawing_Quadrangulate
)

pxr_install_test_dir(
    SRC testenv/testHdStClipPlanes
    DEST testHdStClipPlanes
)

#
# Register a selection of the tests
#

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