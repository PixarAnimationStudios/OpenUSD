set(PXR_PREFIX pxr/usdImaging)
set(PXR_PACKAGE usdImagingGL)

if (NOT ${PXR_BUILD_GPU_SUPPORT})
    message(STATUS
        "Skipping ${PXR_PACKAGE} because PXR_BUILD_GPU_SUPPORT is OFF")
    return()
endif()

# Note: both python include directives are included for compatibility.
pxr_library(usdImagingGL
    LIBRARIES
        gf
        tf
        plug
        trace
        vt
        work
        hio
        garch
        glf
        hd
        hdsi
        hdx
        pxOsd
        sdf
        sdr
        usd
        usdGeom
        usdHydra
        usdShade
        usdImaging
        ar
        ${Boost_PYTHON_LIBRARY}
        ${PYTHON_LIBRARIES}
        ${TBB_tbb_LIBRARY}

    INCLUDE_DIRS
        ${PYTHON_INCLUDE_PATH}
        ${PYTHON_INCLUDE_DIRS}
        ${TBB_INCLUDE_DIRS}

    PUBLIC_CLASSES
        engine
        renderParams

    PUBLIC_HEADERS
        api.h
        rendererSettings.h
        version.h

    PRIVATE_HEADERS
        unitTestGLDrawing.h

    PYTHON_CPPFILES
        moduleDeps.cpp

    PYMODULE_CPPFILES
        module.cpp
        wrapEngine.cpp
        wrapRenderParams.cpp
        wrapRendererCommands.cpp
        wrapRendererSettings.cpp

    PYMODULE_FILES
        __init__.py

    RESOURCE_FILES
        plugInfo.json
)

#
# C++ tests - Only build on all platforms for now.
# 
set(TEST_LIBS
        "arch"
        "garch"
        "glf"
        "hd"
        "sdf"
        "tf"
        "usd"
        "usdUtils"
        "usdImaging"
        "usdImagingGL")

pxr_build_test(testUsdImagingGLBasicDrawing
    LIBRARIES
        ${TEST_LIBS}
    CPPFILES
        unitTestGLDrawing.cpp
        testenv/testUsdImagingGLBasicDrawing.cpp
)

pxr_build_test(testUsdImagingGLBatchedDrawing
    LIBRARIES
        ${TEST_LIBS}
    CPPFILES
        unitTestGLDrawing.cpp
        testenv/testUsdImagingGLBatchedDrawing.cpp
)

pxr_build_test(testUsdImagingGLHighlight
    LIBRARIES
        ${TEST_LIBS}
    CPPFILES
        unitTestGLDrawing.cpp
        testenv/testUsdImagingGLHighlight.cpp
)

pxr_build_test(testUsdImagingGLPickAndHighlight
    LIBRARIES
        ${TEST_LIBS}
    CPPFILES
        unitTestGLDrawing.cpp
        testenv/testUsdImagingGLPickAndHighlight.cpp
)

pxr_build_test(testUsdImagingGLPopOut
    LIBRARIES
        ${TEST_LIBS}
    CPPFILES
        unitTestGLDrawing.cpp
        testenv/testUsdImagingGLPopOut.cpp
)

pxr_build_test(testUsdImagingGLPurpose
    LIBRARIES
        ${TEST_LIBS}
    CPPFILES
        unitTestGLDrawing.cpp
        testenv/testUsdImagingGLPurpose.cpp
)

pxr_build_test(testUsdImagingGLResync
    LIBRARIES
        ${TEST_LIBS}
    CPPFILES
        unitTestGLDrawing.cpp
        testenv/testUsdImagingGLResync.cpp
)

pxr_build_test(testUsdImagingGLSurfaceShader
    LIBRARIES
        ${TEST_LIBS}
    CPPFILES
        unitTestGLDrawing.cpp
        testenv/testUsdImagingGLSurfaceShader.cpp
)

pxr_build_test(testUsdImagingGLTaskController
    LIBRARIES
        ${TEST_LIBS}
    CPPFILES
        unitTestGLDrawing.cpp
        testenv/testUsdImagingGLTaskController.cpp
)

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

# usdImagingGL tests require plugin mechanism to find different renderer
# plugins, which we do not support for static build configurations due to 
# potential conflict in client applications linking against static build USD
if (NOT BUILD_SHARED_LIBS)
    message(STATUS "Skipping ${PXR_PACKAGE} tests on static build configuration")
    return()
endif()

#
# Install tests that don't depend on build configuration or external libraries
# (such as MaterialX, OpenVDB, PTEX).
#

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLBasicDrawing
    DEST testUsdImagingGLBasicDrawing
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLBasicDrawingNonBindless
    DEST testUsdImagingGLBasicDrawingNonBindless
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLBasicLighting
    DEST testUsdImagingGLBasicLighting
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLCurves
    DEST testUsdImagingGLCurves
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLPoints
    DEST testUsdImagingGLPoints
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLDomeLight
    DEST testUsdImagingGLDomeLight
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLDoubleSided
    DEST testUsdImagingGLDoubleSided
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLExtents
    DEST testUsdImagingGLExtents
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLImplicits
    DEST testUsdImagingGLImplicits
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLHoleFaces
    DEST testUsdImagingGLHoleFaces
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLSubdivisionOsd3
    DEST testUsdImagingGLSubdivisionOsd3
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLComplexityOsd3
    DEST testUsdImagingGLComplexityOsd3
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLBasisCurvesWithColor
    DEST testUsdImagingGLBasisCurvesWithColor
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLBasisCurvesWithScale
    DEST testUsdImagingGLBasisCurvesWithScale
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLBasisCurvesWithNormals
    DEST testUsdImagingGLBasisCurvesWithNormals
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLSubdivisionSchemeNone
    DEST testUsdImagingGLSubdivisionSchemeNone
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLVaryingTopology
    DEST testUsdImagingGLVaryingTopology
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLVaryingSharedMeshTopology
    DEST testUsdImagingGLVaryingSharedMeshTopology
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLEmptyMesh
    DEST testUsdImagingGLEmptyMesh
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLTetMesh
    DEST testUsdImagingGLTetMesh
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLTetMesh
    DEST testUsdImagingGLTetMesh_SceneIndex
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLPrimitiveDrawing
    DEST testUsdImagingGLPrimitiveDrawing
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLFlatShading
    DEST testUsdImagingGLFlatShading
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLShadeMaterial
    DEST testUsdImagingGLShadeMaterial
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLShadeMaterial_Delegate
    DEST testUsdImagingGLShadeMaterial_Delegate
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLSphere
    DEST testUsdImagingGLSphere
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLSphere
    DEST testUsdImagingGLSphereWithFloatRadius
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLNurbsCurves
    DEST testUsdImagingGLNurbsCurves
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLNurbsPatch
    DEST testUsdImagingGLNurbsPatch
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLFramingBBox
    DEST testUsdImagingGLFramingBBox
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLInstancing
    DEST testUsdImagingGLInstancing
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLInstancing
    DEST testUsdImagingGLInstancing_SceneIndex
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLInstancing
    DEST testUsdImagingGLInstancing_DeprecatedPrimvars
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLInstancing
    DEST testUsdImagingGLInstancing_DeprecatedPrimvars_SceneIndex
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLPickAndHighlight
    DEST testUsdImagingGLPickAndHighlight
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLPickAndHighlight
    DEST testUsdImagingGLPickAndHighlight_SceneIndex
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLHighlight
    DEST testUsdImagingGLHighlight
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLSurfaceShader
    DEST testUsdImagingGLSurfaceShader
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLShadeBadTexture
    DEST testUsdImagingGLShadeBadTexture
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLShadeBool
    DEST testUsdImagingGLShadeBool
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLPointInstancer
    DEST testUsdImagingGLPointInstancer
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLPointInstancer
    DEST testUsdImagingGLPointInstancer_SceneIndex
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLResync
    DEST testUsdImagingGLResync
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLPurpose
    DEST testUsdImagingGLPurpose
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLPrimvarSharing
    DEST testUsdImagingGLPrimvarSharing
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLColorOpacityPrimvar
    DEST testUsdImagingGLColorOpacityPrimvar
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLTriangleSubdivisionRuleOsd3
    DEST testUsdImagingGLTriangleSubdivisionRuleOsd3
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLWireframeOpacity
    DEST testUsdImagingGLWireframeOpacity
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLCards
    DEST testUsdImagingGLCards
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLMeshQuadrangulation
    DEST testUsdImagingGLMeshQuadrangulation
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLSkeleton
    DEST testUsdImagingGLSkeleton
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLSkelBlendShapes
    DEST testUsdImagingGLSkelBlendShapes
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLNormals
    DEST testUsdImagingGLNormals
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLPreviewSurface
    DEST testUsdImagingGLPreviewSurface
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLPremultiplyAlpha
    DEST testUsdImagingGLPremultiplyAlpha
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLInstancingWithMaterials
    DEST testUsdImagingGLInstancingWithMaterials
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLInstancingWithMaterials
    DEST testUsdImagingGLInstancingWithMaterials_SceneIndex
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLFaceVarying
    DEST testUsdImagingGLFaceVarying
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLShadingPrimvars
    DEST testUsdImagingGLShadingPrimvars
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLTextureWrap
    DEST testUsdImagingGLTextureWrap
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLUsdLux
    DEST testUsdImagingGLUsdLux
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLUsdUdims
    DEST testUsdImagingGLUsdUdims
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLUsdSkel
    DEST testUsdImagingGLUsdSkel
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLPopOut
    DEST testUsdImagingGLPopOut
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLMaterialTag
    DEST testUsdImagingGLMaterialTag
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLMaterialStrengthOrder
    DEST testUsdImagingGLMaterialStrengthOrder
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLInstancePrimvars
    DEST testUsdImagingGLInstancePrimvars
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLSimpleVolumes
    DEST testUsdImagingGLSimpleVolumes
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLVolumeMaterial
    DEST testUsdImagingGLVolumeMaterial
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLBasisCurvesVaryingTopology
    DEST testUsdImagingGLBasisCurvesVaryingTopology
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLAnimatedCamera
    DEST testUsdImagingGLAnimatedCamera
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLClippingPlanes
    DEST testUsdImagingGLClippingPlanes
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLAnimatedLights
    DEST testUsdImagingGLAnimatedLights
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLSdr
    DEST testUsdImagingGLSdr
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLInvalidInfoId
    DEST testUsdImagingGLInvalidInfoId
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLInheritedPurpose
    DEST testUsdImagingGLInheritedPurpose
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLInvalidUniversalSurface
    DEST testUsdImagingGLInvalidUniversalSurface
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLInvalidUniversalSurface
    DEST testUsdImagingGLInvalidUniversalSurface_SceneIndex
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLFallbackPrimTypes
    DEST testUsdImagingGLFallbackPrimTypes
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLPrimvarProcessing
    DEST testUsdImagingGLPrimvarProcessing
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLComposite
    DEST testUsdImagingGLComposite
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLPresentDisabled
    DEST testUsdImagingGLPresentDisabled
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLFraming
    DEST testUsdImagingGLFraming
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLHWFaceCulling
    DEST testUsdImagingGLHWFaceCulling
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLMipmap
    DEST testUsdImagingGLMipmap
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLAovVisualization
    DEST testUsdImagingGLAovVisualization
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLPrimvarRefinement
    DEST testUsdImagingGLPrimvarRefinement
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLDisplacement
    DEST testUsdImagingGLDisplacement
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLGeomSubsets
    DEST testUsdImagingGLGeomSubsets
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLGeomSubsets
    DEST testUsdImagingGLGeomSubsets_SceneIndex
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLInvalidMaterial
    DEST testUsdImagingGLInvalidMaterial
)

pxr_install_test_dir(
    SRC testenv/testUsdImagingGLDomeLightTaskController
    DEST testUsdImagingGLDomeLightTaskController
)

#
# Register tests that don't depend on build configuration or external libraries
# (such as MaterialX, OpenVDB, PTEX).
#
pxr_register_test(testUsdImagingGLBasicDrawing
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -stage basicDrawing/basicDrawing.usda -write testUsdImagingGLBasicDrawing.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLBasicDrawing.png
    FAIL 0.2
    FAIL_PERCENT 0.5
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    ENV
        HGIGL_ENABLE_BINDLESS_TEXTURE=1
)

pxr_register_test(testUsdImagingGLBasicDrawing_refined
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -complexity 1.1 -stage basicDrawing/basicDrawing.usda -write testUsdImagingGLBasicDrawing_refined.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLBasicDrawing_refined.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLBasicDrawing
    ENV
        HGIGL_ENABLE_BINDLESS_TEXTURE=1
)

pxr_register_test(testUsdImagingGLBasicDrawing_leftHanded
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -shading wireOnSurface -stage basicDrawing/leftHanded.usda -write testUsdImagingGLBasicDrawing_leftHanded.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLBasicDrawing_leftHanded.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLBasicDrawing
    ENV
        HGIGL_ENABLE_BINDLESS_TEXTURE=1
)

pxr_register_test(testUsdImagingGLBasicDrawing_clipped
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -stage basicDrawing/basicDrawing.usda -clipPlane 1 0 0 0 -frameAll -write testUsdImagingGLBasicDrawing_clipped.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLBasicDrawing_clipped.png
    FAIL 1
    FAIL_PERCENT 0.9
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLBasicDrawing
    ENV
        HGIGL_ENABLE_BINDLESS_TEXTURE=1
)

pxr_register_test(testUsdImagingGLBasicDrawing_id
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -idRender -stage basicDrawing/shaders.usd -frameAll -write testUsdImagingGLBasicDrawing_id.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLBasicDrawing_id.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLBasicDrawing
    ENV
        HGIGL_ENABLE_BINDLESS_TEXTURE=1
)

pxr_register_test(testUsdImagingGLBasicDrawing_subdivTags
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -stage subdivTags.usda -frameAll -lighting -write testUsdImagingGLBasicDrawing_subdivTags.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLBasicDrawing_subdivTags.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLBasicDrawing
    ENV
        HGIGL_ENABLE_BINDLESS_TEXTURE=1
)

pxr_register_test(testUsdImagingGLBasicDrawing_subdivTagsRefined
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -stage subdivTags.usda -frameAll -lighting -complexity 1.4 -write testUsdImagingGLBasicDrawing_subdivTagsRefined.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLBasicDrawing_subdivTagsRefined.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLBasicDrawing
    ENV
        HGIGL_ENABLE_BINDLESS_TEXTURE=1
)

pxr_register_test(testUsdImagingGLBasicDrawing_inputShaderEncoding
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -stage basicDrawing/newInputShaderEncoding.usd -frameAll -lighting -write testUsdImagingGLBasicDrawing_inputShaderEncoding.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLBasicDrawing_inputShaderEncoding.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLBasicDrawing
    ENV
        HGIGL_ENABLE_BINDLESS_TEXTURE=1
)

pxr_register_test(testUsdImagingGLBasicDrawing_textureChannels
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -stage basicDrawing/textureChannels.usd -frameAll -lighting -write testUsdImagingGLBasicDrawing_textureChannels.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLBasicDrawing_textureChannels.png
    FAIL 0.01
    FAIL_PERCENT 1
    WARN 0.02
    WARN_PERCENT 0.05
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLBasicDrawing
    ENV
        HGIGL_ENABLE_BINDLESS_TEXTURE=1
)

pxr_register_test(testUsdImagingGLBasicDrawingNonBindless_batch
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -lighting -offscreen -stage shaders.usda -frameAll -write testUsdImagingGLBasicDrawingNonBindless_batch.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLBasicDrawingNonBindless_batch.png
    FAIL 1
    FAIL_PERCENT 1.1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLBasicDrawingNonBindless
    ENV
        HGIGL_ENABLE_BINDLESS_TEXTURE=0
)

pxr_register_test(testUsdImagingGLBasicLighting
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -stage basicDrawing.usda -write testUsdImagingGLBasicLighting.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLBasicLighting.png
    FAIL 0.5
    FAIL_PERCENT 0.1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLBasicLighting
)

pxr_register_test(testUsdImagingGLBasicLighting_refined
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -complexity 1.1 -lighting -stage basicDrawing.usda -write testUsdImagingGLBasicLighting_refined.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLBasicLighting_refined.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLBasicLighting
)

pxr_register_test(testUsdImagingGLCurves
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -frameAll -stage basicCurves.usda -write testUsdImagingGLCurves.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLCurves.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLCurves
)

pxr_register_test(testUsdImagingGLCurves_refined
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -frameAll -complexity 1.1 -stage basicCurves.usda -write testUsdImagingGLCurves_refined.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLCurves_refined.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLCurves
)

pxr_register_test(testUsdImagingGLCurves_moreCurves
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -frameAll -stage moreCurves.usda -camlight -shading smooth -write testUsdImagingGL_moreCurves.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGL_moreCurves.png
    FAIL 1
    FAIL_PERCENT 0.5
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLCurves
)

pxr_register_test(testUsdImagingGLCurves_moreCurves_refined
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -frameAll -complexity 1.1 -stage moreCurves.usda -camlight -shading smooth -write testUsdImagingGL_moreCurves_refined.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGL_moreCurves_refined.png
    FAIL 1
    FAIL_PERCENT 0.9
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLCurves
)

pxr_register_test(testUsdImagingGLCurves_moreCurves_faketubes
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -frameAll -complexity 1.2 -stage moreCurves.usda -camlight -shading smooth -write testUsdImagingGL_moreCurves_faketubes.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGL_moreCurves_faketubes.png
    FAIL 1
    FAIL_PERCENT 0.9
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLCurves
)

pxr_register_test(testUsdImagingGLCurves_moreCurves_tubes
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -frameAll -complexity 1.3 -stage moreCurves.usda -camlight  -shading smooth -write testUsdImagingGL_moreCurves_tubes.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGL_moreCurves_tubes.png
    FAIL 1
    FAIL_PERCENT 0.9
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLCurves
)

pxr_register_test(testUsdImagingGLCurves_pinnedCurves
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -frameAll -complexity 1.3 -stage pinnedCurves.usda -camlight  -shading smooth -write testUsdImagingGL_pinnedCurves.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGL_pinnedCurves.png
    FAIL 1
    FAIL_PERCENT 0.2
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLCurves
)

pxr_register_test(testUsdImagingGLPoints
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -frameAll -stage points.usda -widgetSize 640 480 -write testUsdImagingGLPoints.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLPoints.png
    FAIL 1
    FAIL_PERCENT 0.9
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPoints
)

pxr_register_test(testUsdImagingGLPoints_clipped
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -stage points.usda -widgetSize 640 480 -clipPlane -1 0 0 0 -frameAll -write testUsdImagingGLPoints_clipped.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLPoints_clipped.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPoints
)

pxr_register_test(testUsdImagingGLPoints_zoom
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -stage points.usda -dataWindow 0 0 639 479 -displayWindow -320 -240 1280 960 -frameAll -write testUsdImagingGLPoints_zoom.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLPoints_zoom.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPoints
)

pxr_register_test(testUsdImagingGLPoints_zoom_pan
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -stage points.usda -dataWindow 0 0 639 479 -displayWindow -620 -240 1280 960 -frameAll -write testUsdImagingGLPoints_zoom_pan.png"
    IMAGE_DIFF_COMPARE
    testUsdImagingGLPoints_zoom_pan.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPoints
)

pxr_register_test(testUsdImagingGLDomeLight_ZupFront
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -frameAll -offscreen -lighting -sceneLights -shading smooth -complexity 1.3 -camera /Scene/FrontCamera -stage domeLightZup.usda -write testUsdImagingGLDomeLight_ZupFront.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLDomeLight_ZupFront.png
    FAIL 1
    FAIL_PERCENT 0.005
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLDomeLight
)

pxr_register_test(testUsdImagingGLDomeLight_ZupBack
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -frameAll -offscreen -lighting -sceneLights -shading smooth -complexity 1.3 -camera /Scene/BackCamera -stage domeLightZup.usda -write testUsdImagingGLDomeLight_ZupBack.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLDomeLight_ZupBack.png
    FAIL 1
    FAIL_PERCENT 0.006
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLDomeLight
)

pxr_register_test(testUsdImagingGLDomeLight_YupFront
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -frameAll -offscreen -lighting -sceneLights -shading smooth -complexity 1.3 -camera /Scene/FrontCamera -stage domeLightYup.usda -write testUsdImagingGLDomeLight_YupFront.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLDomeLight_YupFront.png
    FAIL 1
    FAIL_PERCENT 0.002
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLDomeLight
)

pxr_register_test(testUsdImagingGLDomeLight_YupBack
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -frameAll -offscreen -lighting -sceneLights -shading smooth -complexity 1.3 -camera /Scene/BackCamera -stage domeLightYup.usda -write testUsdImagingGLDomeLight_YupBack.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLDomeLight_YupBack.png
    FAIL 1
    FAIL_PERCENT 0.003
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLDomeLight
)

pxr_register_test(testUsdImagingGLDomeLight_multi
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -frameAll -offscreen -lighting -sceneLights -shading smooth -complexity 1.3 -stage multiDomeLights.usda -write testUsdImagingGLDomeLight_multi.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLDomeLight_multi.png
    FAIL 1
    FAIL_PERCENT 0.001
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLDomeLight
)

pxr_register_test(testUsdImagingGLDomeLight_metallic
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -frameAll -offscreen -lighting -sceneLights -shading smooth -complexity 1.3 -stage sphereMetallic.usda -write testUsdImagingGLDomeLight_metallic.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLDomeLight_metallic.png
    FAIL 1
    FAIL_PERCENT 0.001
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLDomeLight
)

pxr_register_test(testUsdImagingGLDomeLight_rough
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -frameAll -offscreen -lighting -sceneLights -shading smooth -complexity 1.3 -stage sphereRough.usda -write testUsdImagingGLDomeLight_rough.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLDomeLight_rough.png
    FAIL 1
    FAIL_PERCENT 0.005
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLDomeLight
)

pxr_register_test(testUsdImagingGLDomeLight_grid
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -frameAll -offscreen -lighting -sceneLights -shading smooth -complexity 1.3 -stage sphereGrid.usda -write testUsdImagingGLDomeLight_grid.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLDomeLight_grid.png
    FAIL 1
    FAIL_PERCENT 0.5
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLDomeLight
)

pxr_register_test(testUsdImagingGLDomeLight_metallic_specwf
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -frameAll -offscreen -lighting -sceneLights -shading smooth -complexity 1.3 -stage sphereMetallic_specwf.usda -write testUsdImagingGLDomeLight_metallic_specwf.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLDomeLight_metallic_specwf.png
    FAIL 1
    FAIL_PERCENT 0.4
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLDomeLight
)

pxr_register_test(testUsdImagingGLDomeLight_grid_specwf
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -frameAll -offscreen -lighting -sceneLights -shading smooth -complexity 1.3 -stage sphereGrid_specwf.usda -write testUsdImagingGLDomeLight_grid_specwf.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLDomeLight_grid_specwf.png
    FAIL 1
    FAIL_PERCENT 0.6
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLDomeLight
)

pxr_register_test(testUsdImagingGLDomeLight_invis
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -frameAll -offscreen -lighting -sceneLights -shading smooth -complexity 1.3 -stage invisDome.usda -write testUsdImagingGLDomeLight_invis.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLDomeLight_invis.png
    FAIL 0.01
    FAIL_PERCENT 0.2
    WARN 0.02
    WARN_PERCENT 0.005
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLDomeLight
)

pxr_register_test(testUsdImagingGLDoubleSided
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -stage doubleSided2.usda -write testUsdImagingGLDoubleSided.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLDoubleSided.png
    FAIL 1
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLDoubleSided
)

pxr_register_test(testUsdImagingGLExtents
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -frameAll -offscreen -stage testExtents.usda -write testUsdImagingGLExtents.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLExtents.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLExtents
)

pxr_register_test(testUsdImagingGLImplicits
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -shading wireOnSurface -frameAll -stage implicits.usda -write testUsdImagingGLImplicits.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLImplicits.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLImplicits
)

pxr_register_test(testUsdImagingGLImplicits_refined
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -shading wireOnSurface -complexity 1.1 -frameAll -stage implicits.usda -write testUsdImagingGLImplicits_refined.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLImplicits_refined.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLImplicits
)

pxr_register_test(testUsdImagingGLHoleFaces
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -stage holeFaces.usda -write testUsdImagingGLHoleFaces.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLHoleFaces.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLHoleFaces
)

pxr_register_test(testUsdImagingGLSubdivision_1_1
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -complexity 1.1 -offscreen -lighting -stage subdivMesh.usda -write testUsdImagingGLSubdivision_1_1.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLSubdivision_1_1.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLSubdivisionOsd3
)

pxr_register_test(testUsdImagingGLSubdivision_1_2
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -complexity 1.2 -offscreen -lighting -stage subdivMesh.usda -write testUsdImagingGLSubdivision_1_2.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLSubdivision_1_2.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLSubdivisionOsd3
)

pxr_register_test(testUsdImagingGLSubdivision_1_3
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -complexity 1.3 -offscreen -lighting -stage subdivMesh.usda -write testUsdImagingGLSubdivision_1_3.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLSubdivision_1_3.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLSubdivisionOsd3
)

pxr_register_test(testUsdImagingGLSubdivision_lefthanded_1_1
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -complexity 1.1 -frameAll -cullStyle back -offscreen -lighting -stage lefthandedMesh.usda -write testUsdImagingGLSubdivision_lefthanded_1_1.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLSubdivision_lefthanded_1_1.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLSubdivisionOsd3
)

pxr_register_test(testUsdImagingGLSubdivision_lefthanded_1_2
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -complexity 1.2 -frameAll -cullStyle back -offscreen -lighting -stage lefthandedMesh.usda -write testUsdImagingGLSubdivision_lefthanded_1_2.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLSubdivision_lefthanded_1_2.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLSubdivisionOsd3
)

pxr_register_test(testUsdImagingGLComplexityOsd3
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -complexities 1.0 1.1 1.2 -offscreen -lighting -stage subdivMesh.usda -write testUsdImagingGLComplexity.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLComplexity_1.png
        testUsdImagingGLComplexity_1.1.png
        testUsdImagingGLComplexity_1.2.png
    FAIL .05
    FAIL_PERCENT .03
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLComplexityOsd3
)

pxr_register_test(testUsdImagingGLBasisCurvesWithColor
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -complexities 1.0 1.1 1.2 1.3 -offscreen -frameAll -stage curveWithVertexColor.usda -write testUsdImagingGLBasisCurvesWithColor.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLBasisCurvesWithColor_1.png
        testUsdImagingGLBasisCurvesWithColor_1.1.png
        testUsdImagingGLBasisCurvesWithColor_1.2.png
        testUsdImagingGLBasisCurvesWithColor_1.3.png
    FAIL .05
    FAIL_PERCENT .03
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLBasisCurvesWithColor
)

pxr_register_test(testUsdImagingGLBasisCurvesWithScale
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -complexities 1.1 -offscreen -lighting -stage curve-scales-flat.usd -write testUsdImagingGLBasisCurvesWithScale.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLBasisCurvesWithScale_1.1.png
    FAIL .05
    FAIL_PERCENT .03
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLBasisCurvesWithScale
)

pxr_register_test(testUsdImagingGLBasisCurvesWithNormals
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -complexities 1.1 -frameAll -offscreen -lighting -stage curve-authored-normals.usd -write testUsdImagingGLBasisCurvesWithNormals.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLBasisCurvesWithNormals_1.1.png
    FAIL .05
    FAIL_PERCENT .03
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLBasisCurvesWithNormals
)

pxr_register_test(testUsdImagingGLSubdivisionSchemeNone
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -frameAll -complexities 1.0 1.3 -offscreen -lighting -stage ring.usda -write testUsdImagingGLSubdivisionSchemeNone.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLSubdivisionSchemeNone_1.png
        testUsdImagingGLSubdivisionSchemeNone_1.3.png
    FAIL 0.3
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLSubdivisionSchemeNone
)

pxr_register_test(testUsdImagingGLSubdivisionSchemeNone_NoLights
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -frameAll -complexities 1.0 1.3 -offscreen -shading wireOnSurface -stage ring.usda -write testUsdImagingGLSubdivisionSchemeNone_NoLights.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLSubdivisionSchemeNone_NoLights_1.png
        testUsdImagingGLSubdivisionSchemeNone_NoLights_1.3.png
    FAIL 0.2
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLSubdivisionSchemeNone
)

pxr_register_test(testUsdImagingGLVaryingTopology
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -frameAll -offscreen -lighting -times 1 2 3 -stage varying.usda -write testUsdImagingGLVaryingTopology.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLVaryingTopology_001.png
        testUsdImagingGLVaryingTopology_002.png
        testUsdImagingGLVaryingTopology_003.png
    FAIL .05
    FAIL_PERCENT .03
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLVaryingTopology
)

pxr_register_test(testUsdImagingGLVaryingSharedMeshTopology
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -frameAll -offscreen -camlight -times 1 2 3 1 2 3 -stage meshVaryingTopologyWithSharing.usda -camera /MainCamera -write testUsdImagingGLVaryingSharedMeshTopology.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLVaryingSharedMeshTopology_001.png
        testUsdImagingGLVaryingSharedMeshTopology_002.png
        testUsdImagingGLVaryingSharedMeshTopology_003.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLVaryingSharedMeshTopology
)

pxr_register_test(testUsdImagingGLEmptyMesh
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -frameAll -offscreen -lighting -times 1 5 3 7 -stage test.usda -write testUsdImagingGLEmptyMesh.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLEmptyMesh_001.png
        testUsdImagingGLEmptyMesh_005.png
        testUsdImagingGLEmptyMesh_003.png
        testUsdImagingGLEmptyMesh_007.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLEmptyMesh
)

pxr_register_test(testUsdImagingGLTetMesh
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -lighting -offscreen -times 0 10 -stage tetMesh.usda -camera /Camera -write testUsdImagingGLTetMesh.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLTetMesh_000.png
        testUsdImagingGLTetMesh_010.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLTetMesh
)

pxr_register_test(testUsdImagingGLTetMesh_SceneIndex
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -lighting -offscreen -times 0 10 -stage tetMesh.usda -camera /Camera -write testUsdImagingGLTetMesh.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLTetMesh_000.png
        testUsdImagingGLTetMesh_010.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLTetMesh_SceneIndex
    ENV
        USDIMAGINGGL_ENGINE_ENABLE_SCENE_INDEX=true
)

pxr_register_test(testUsdImagingGLPrimitiveDrawing
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -frameAll -stage primitives.usda -write testUsdImagingGLPrimitiveDrawing.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLPrimitiveDrawing.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPrimitiveDrawing
)

pxr_register_test(testUsdImagingGLFlatShading
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -frameAll -lighting -shading flat -stage primitives.usda -write testUsdImagingGLFlatShading.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLFlatShading.png
    FAIL 0.1
    FAIL_PERCENT 1
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLFlatShading
)

pxr_register_test(testUsdImagingGLShadeMaterial
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -frameAll -stage coloredCubes.usda -write testUsdImagingGLShadeMaterial.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLShadeMaterial.png
    FAIL 0.3
    FAIL_PERCENT 0.9
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLShadeMaterial
    ENV
        USDIMAGINGGL_ENGINE_ENABLE_SCENE_INDEX=true
)

pxr_register_test(testUsdImagingGLShadeMaterial_Delegate
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -frameAll -stage coloredCubes.usda -write testUsdImagingGLShadeMaterial.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLShadeMaterial.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLShadeMaterial_Delegate
    ENV
        USDIMAGINGGL_ENGINE_ENABLE_SCENE_INDEX=false
)

pxr_register_test(testUsdImagingGLSphere
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -frameAll -stage sphere.usda -camera /main_cam -times 1 6 10 -write testUsdImagingGLSphere.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLSphere_001.png
        testUsdImagingGLSphere_010.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLSphere
)

# There is a bug for sphere with float radius:
# authoring a float radius doesn't actually pick up the authored float radius but the schema default value.
# In the test, the attribute has animation and at frame 6, the value is the schema default value.
# Thus, the test is passing exactly for frame 6.
pxr_register_test(testUsdImagingGLSphereWithFloatRadius
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -frameAll -stage sphereWithFloatRadius.usda -camera /main_cam -times 1 6 10 -write testUsdImagingGLSphere.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLSphere_006.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLSphere
)

pxr_register_test(testUsdImagingGLNurbsCurves
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -frameAll -offscreen -times 0 50 150 -stage testCurves.usda -write testUsdImagingGLNurbsCurves.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLNurbsCurves_000.png
        testUsdImagingGLNurbsCurves_050.png
        testUsdImagingGLNurbsCurves_150.png
    FAIL .05
    FAIL_PERCENT .03
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLNurbsCurves
)

pxr_register_test(testUsdImagingGLNurbsCurves_refined
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -frameAll -offscreen -times 0 50 150 -complexity 1.1 -stage testCurves.usda -write testUsdImagingGLNurbsCurves_refined.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLNurbsCurves_refined_000.png
        testUsdImagingGLNurbsCurves_refined_050.png
        testUsdImagingGLNurbsCurves_refined_150.png
    FAIL .05
    FAIL_PERCENT .03
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLNurbsCurves
)

pxr_register_test(testUsdImagingGLNurbsPatch
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -frameAll -offscreen -lighting -stage bendyNurbs.usda -camera /main_cam -times 1 5 10 -write testUsdImagingGLNurbsPatch.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLNurbsPatch_001.png
        testUsdImagingGLNurbsPatch_005.png
        testUsdImagingGLNurbsPatch_010.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLNurbsPatch
)

pxr_register_test(testUsdImagingGLFramingBBox_proxy
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -frameAll -stage framing.usda -write testUsdImagingGLFramingBBox_proxy.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLFramingBBox_proxy.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLFramingBBox
)

pxr_register_test(testUsdImagingGLFramingBBox_proxy_render
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -frameAll -stage framing.usda -renderPurpose show -write testUsdImagingGLFramingBBox_proxy_render.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLFramingBBox_proxy_render.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLFramingBBox
)

pxr_register_test(testUsdImagingGLInstancing_instancedCubes
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -shading smooth -frameAll -stage instancedCubes.usda -write testUsdImagingGLInstancing_instancedCubes.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLInstancing_instancedCubes.png
    FAIL 0.01
    FAIL_PERCENT 0.005
    WARN 0.02
    WARN_PERCENT 0.0025
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLInstancing
)

pxr_register_test(testUsdImagingGLInstancing_instancedCubes2
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -frameAll -stage instancedCubes2.usda -write testUsdImagingGLInstancing_instancedCubes2.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLInstancing_instancedCubes2.png
    FAIL 0.01
    FAIL_PERCENT 0.005
    WARN 0.02
    WARN_PERCENT 0.0025
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLInstancing
)

pxr_register_test(testUsdImagingGLInstancing_instancedCubes2NoInstancing
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -frameAll -stage instancedCubes2NoInstancing.usda -write testUsdImagingGLInstancing_instancedCubes2NoInstancing.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLInstancing_instancedCubes2NoInstancing.png
    FAIL 0.01
    FAIL_PERCENT 0.005
    WARN 0.02
    WARN_PERCENT 0.0025
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLInstancing
)

pxr_register_test(testUsdImagingGLInstancing_nestedInstancedCubes
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -frameAll -stage nestedInstancedCubes.usda -write testUsdImagingGLInstancing_nestedInstancedCubes.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLInstancing_nestedInstancedCubes.png
    FAIL 0.01
    FAIL_PERCENT 0.005
    WARN 0.02
    WARN_PERCENT 0.0025
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLInstancing
)

pxr_register_test(testUsdImagingGLInstancing_ni_pi
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -frameAll -lighting -stage ni_pi.usda -write testUsdImagingGLInstancing_ni_pi.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLInstancing_ni_pi.png
    FAIL 0.01
    FAIL_PERCENT 0.005
    WARN 0.02
    WARN_PERCENT 0.0025
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLInstancing
)

pxr_register_test(testUsdImagingGLInstancing_internal_ni
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -frameAll -lighting -stage internal_ni.usda -write testUsdImagingGLInstancing_internal_ni.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLInstancing_internal_ni.png
    FAIL 0.01
    FAIL_PERCENT 0.005
    WARN 0.02
    WARN_PERCENT 0.0025
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLInstancing
)

pxr_register_test(testUsdImagingGLInstancing_pi_ni_xform
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -camera /main_cam -lighting -stage pi_ni_xform.usda -write testUsdImagingGLInstancing_pi_ni_xform.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLInstancing_pi_ni_xform.png
    FAIL 0.01
    FAIL_PERCENT 0.005
    WARN 0.02
    WARN_PERCENT 0.0025
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLInstancing
)

pxr_register_test(testUsdImagingGLInstancing_SceneIndex_instancedCubes
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -shading smooth -frameAll -stage instancedCubes.usda -write testUsdImagingGLInstancing_instancedCubes.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLInstancing_instancedCubes.png
    FAIL 0.01
    FAIL_PERCENT 0.005
    WARN 0.02
    WARN_PERCENT 0.0025
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLInstancing_SceneIndex
    ENV
        USDIMAGINGGL_ENGINE_ENABLE_SCENE_INDEX=true
)

pxr_register_test(testUsdImagingGLInstancing_SceneIndex_instancedCubes2
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -frameAll -stage instancedCubes2.usda -write testUsdImagingGLInstancing_instancedCubes2.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLInstancing_instancedCubes2.png
    FAIL 0.01
    FAIL_PERCENT 0.005
    WARN 0.02
    WARN_PERCENT 0.0025
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLInstancing_SceneIndex
    ENV
        USDIMAGINGGL_ENGINE_ENABLE_SCENE_INDEX=true
)

pxr_register_test(testUsdImagingGLInstancing_SceneIndex_instancedCubes2NoInstancing
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -frameAll -stage instancedCubes2NoInstancing.usda -write testUsdImagingGLInstancing_instancedCubes2NoInstancing.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLInstancing_instancedCubes2NoInstancing.png
    FAIL 0.01
    FAIL_PERCENT 0.005
    WARN 0.02
    WARN_PERCENT 0.0025
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLInstancing_SceneIndex
    ENV
        USDIMAGINGGL_ENGINE_ENABLE_SCENE_INDEX=true
)

pxr_register_test(testUsdImagingGLInstancing_SceneIndex_nestedInstancedCubes
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -frameAll -stage nestedInstancedCubes.usda -write testUsdImagingGLInstancing_nestedInstancedCubes.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLInstancing_nestedInstancedCubes.png
    FAIL 0.01
    FAIL_PERCENT 0.005
    WARN 0.02
    WARN_PERCENT 0.0025
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLInstancing_SceneIndex
    ENV
        USDIMAGINGGL_ENGINE_ENABLE_SCENE_INDEX=true
)

pxr_register_test(testUsdImagingGLInstancing_SceneIndex_ni_pi
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -frameAll -lighting -stage ni_pi.usda -write testUsdImagingGLInstancing_ni_pi.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLInstancing_ni_pi.png
    FAIL 0.01
    FAIL_PERCENT 0.005
    WARN 0.02
    WARN_PERCENT 0.0025
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLInstancing_SceneIndex
    ENV
        USDIMAGINGGL_ENGINE_ENABLE_SCENE_INDEX=true
)

pxr_register_test(testUsdImagingGLInstancing_SceneIndex_internal_ni
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -frameAll -lighting -stage internal_ni.usda -write testUsdImagingGLInstancing_internal_ni.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLInstancing_internal_ni.png
    FAIL 0.01
    FAIL_PERCENT 0.005
    WARN 0.02
    WARN_PERCENT 0.0025
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLInstancing_SceneIndex
    ENV
        USDIMAGINGGL_ENGINE_ENABLE_SCENE_INDEX=true
)

pxr_register_test(testUsdImagingGLInstancing_SceneIndex_pi_ni_xform
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -camera /main_cam -lighting -stage pi_ni_xform.usda -write testUsdImagingGLInstancing_pi_ni_xform.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLInstancing_pi_ni_xform.png
    FAIL 0.01
    FAIL_PERCENT 0.005
    WARN 0.02
    WARN_PERCENT 0.0025
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLInstancing_SceneIndex
    ENV
        USDIMAGINGGL_ENGINE_ENABLE_SCENE_INDEX=true
)

pxr_register_test(testUsdImagingGLPickAndHighlight
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLPickAndHighlight -offscreen -frameAll -stage instance.usda -write testUsdImagingGLPickAndHighlight.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLPickAndHighlight_000.png
        testUsdImagingGLPickAndHighlight_001.png
        testUsdImagingGLPickAndHighlight_002.png
        testUsdImagingGLPickAndHighlight_003.png
        testUsdImagingGLPickAndHighlight_004.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPickAndHighlight
    ENV
        USDIMAGINGGL_ENGINE_ENABLE_SCENE_INDEX=false
)

pxr_register_test(testUsdImagingGLPickAndHighlight_SceneIndex
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLPickAndHighlight -offscreen -frameAll -stage instance.usda -write testUsdImagingGLPickAndHighlight.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLPickAndHighlight_000.png
        testUsdImagingGLPickAndHighlight_001.png
        testUsdImagingGLPickAndHighlight_002.png
        testUsdImagingGLPickAndHighlight_003.png
        testUsdImagingGLPickAndHighlight_004.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPickAndHighlight_SceneIndex
    ENV
        USD_IMAGING_GL_PICK_TEST_SKIP_INSTANCER_DETAILS=true
        USDIMAGINGGL_ENGINE_ENABLE_SCENE_INDEX=true
)

pxr_register_test(testUsdImagingGLHighlight
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLHighlight -offscreen -frameAll -stage instance.usda -write testUsdImagingGLHighlight.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLHighlight_000.png
        testUsdImagingGLHighlight_001.png
        testUsdImagingGLHighlight_002.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLHighlight
)

pxr_register_test(testUsdImagingGLHighlight_pi
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLHighlight -offscreen -frameAll -stage pi.usda -write testUsdImagingGLHighlight_pi.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLHighlight_pi_000.png
        testUsdImagingGLHighlight_pi_001.png
        testUsdImagingGLHighlight_pi_002.png
        testUsdImagingGLHighlight_pi_003.png
        testUsdImagingGLHighlight_pi_004.png
        testUsdImagingGLHighlight_pi_005.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLHighlight
)

pxr_register_test(testUsdImagingGLHighlight_pi_ni
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLHighlight -offscreen -frameAll -stage pi_ni.usda -write testUsdImagingGLHighlight_pi_ni.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLHighlight_pi_ni_000.png
        testUsdImagingGLHighlight_pi_ni_001.png
        testUsdImagingGLHighlight_pi_ni_002.png
        testUsdImagingGLHighlight_pi_ni_003.png
        testUsdImagingGLHighlight_pi_ni_004.png
        testUsdImagingGLHighlight_pi_ni_005.png
        testUsdImagingGLHighlight_pi_ni_006.png
        testUsdImagingGLHighlight_pi_ni_007.png
        testUsdImagingGLHighlight_pi_ni_008.png
        testUsdImagingGLHighlight_pi_ni_009.png
        testUsdImagingGLHighlight_pi_ni_010.png
        testUsdImagingGLHighlight_pi_ni_011.png
        testUsdImagingGLHighlight_pi_ni_012.png
        testUsdImagingGLHighlight_pi_ni_013.png
        testUsdImagingGLHighlight_pi_ni_014.png
        testUsdImagingGLHighlight_pi_ni_015.png
        testUsdImagingGLHighlight_pi_ni_016.png
        testUsdImagingGLHighlight_pi_ni_017.png
        testUsdImagingGLHighlight_pi_ni_018.png
        testUsdImagingGLHighlight_pi_ni_019.png
    FAIL .05
    FAIL_PERCENT .03
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLHighlight
)

pxr_register_test(testUsdImagingGLSurfaceShader_1_2_5
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLSurfaceShader -offscreen -frameAll -stage shader_change.usda -lighting"
    IMAGE_DIFF_COMPARE
        out1.png
        out2.png
        out5.png
    FAIL 1
    FAIL_PERCENT 1.5
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLSurfaceShader
)

pxr_register_test(testUsdImagingGLSurfaceShader_3_4
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLSurfaceShader -offscreen -frameAll -stage shader_change.usda -lighting"
    IMAGE_DIFF_COMPARE
        out3.png
        out4.png
    FAIL 0.01
    FAIL_PERCENT 0.001
    WARN 0.02
    WARN_PERCENT 0.0005
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLSurfaceShader
)

pxr_register_test(testUsdImagingGLShadeBadTexture
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLSurfaceShader -offscreen -frameAll -stage shader_change.usda -lighting"
    IMAGE_DIFF_COMPARE
        out1.png
        out2.png
        out3.png
        out4.png
        out5.png
    FAIL .05
    FAIL_PERCENT .03
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLShadeBadTexture
)

pxr_register_test(testUsdImagingGLShadeBool
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLSurfaceShader -offscreen -frameAll -stage shadebool.usda -lighting"
    IMAGE_DIFF_COMPARE
        out1.png
        out2.png
        out3.png
        out4.png
        out5.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLShadeBool
)

pxr_register_test(testUsdImagingGLPointInstancer_pi_basic
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -frameAll -lighting -camlight -stage pi_basic.usda -times 1 2 -write pi_basic.png"
    IMAGE_DIFF_COMPARE
        pi_basic_001.png
        pi_basic_002.png
    FAIL .05
    FAIL_PERCENT .03
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPointInstancer
)

pxr_register_test(testUsdImagingGLPointInstancer_pi
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -frameAll -lighting -camlight -stage pi.usda -write pi.png"
    IMAGE_DIFF_COMPARE
        pi.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPointInstancer
)

pxr_register_test(testUsdImagingGLPointInstancer_pi_pi
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -frameAll -lighting -camlight -stage pi_pi.usda -write pi_pi.png"
    IMAGE_DIFF_COMPARE
        pi_pi.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPointInstancer
)

pxr_register_test(testUsdImagingGLPointInstancer_pi_pi_which_rot
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -frameAll -lighting -camlight -stage pi_pi_which_rot.usda -write pi_pi_which_rot.png"
    IMAGE_DIFF_COMPARE
        pi_pi_which_rot.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPointInstancer
)

pxr_register_test(testUsdImagingGLPointInstancer_pi_pi2
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -frameAll -lighting -camlight -stage pi_pi2.usda -write pi_pi2.png"
    IMAGE_DIFF_COMPARE
        pi_pi2.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPointInstancer
)

pxr_register_test(testUsdImagingGLPointInstancer_ni_pi
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -frameAll -lighting -camlight -stage ni_pi.usda -write ni_pi.png"
    IMAGE_DIFF_COMPARE
        ni_pi.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPointInstancer
)

pxr_register_test(testUsdImagingGLPointInstancer_pi_ni
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -frameAll -lighting -camlight -stage pi_ni.usda -write pi_ni.png"
    IMAGE_DIFF_COMPARE
        pi_ni.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPointInstancer
)

pxr_register_test(testUsdImagingGLPointInstancer_pi_ni2
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -frameAll -lighting -camlight -stage pi_ni2.usda -write pi_ni2.png"
    IMAGE_DIFF_COMPARE
        pi_ni2.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPointInstancer
)

pxr_register_test(testUsdImagingGLPointInstancer_pi_ni_pi
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -frameAll -lighting -camlight -stage pi_ni_pi.usda -write pi_ni_pi.png"
    IMAGE_DIFF_COMPARE
        pi_ni_pi.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPointInstancer
)

pxr_register_test(testUsdImagingGLPointInstancer_pi_root_prototype
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -frameAll -lighting -camlight -stage pi_root_prototype.usda -write pi_root_prototype.png"
    IMAGE_DIFF_COMPARE
        pi_root_prototype.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPointInstancer
)

pxr_register_test(testUsdImagingGLPointInstancer_pi_sibling_prototype
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -frameAll -lighting -camlight -stage pi_sibling_prototype.usda -write pi_sibling_prototype.png"
    IMAGE_DIFF_COMPARE
        pi_sibling_prototype.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPointInstancer
)

pxr_register_test(testUsdImagingGLPointInstancer_pi_sibling_pi
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -frameAll -lighting -camlight -stage pi_sibling_pi.usda -write pi_sibling_pi.png"
    IMAGE_DIFF_COMPARE
        pi_sibling_pi.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPointInstancer
)

pxr_register_test(testUsdImagingGLPointInstancer_pi_sibling_pi_over
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -frameAll -lighting -camlight -stage pi_sibling_pi_over.usda -write pi_sibling_pi_over.png"
    IMAGE_DIFF_COMPARE
        pi_sibling_pi_over.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPointInstancer
)

pxr_register_test(testUsdImagingGLPointInstancer_pi_varying
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -camlight -translate 0 -10 -40 -clear 0.2 0.2 0.2 1 -stage pi_varying.usda -times 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 18 19 20 21 22 23 24 25 26 27 28 29 30 -write pi_varying.png"
    IMAGE_DIFF_COMPARE
        pi_varying_000.png
        pi_varying_001.png
        pi_varying_002.png
        pi_varying_003.png
        pi_varying_004.png
        pi_varying_005.png
        pi_varying_006.png
        pi_varying_007.png
        pi_varying_008.png
        pi_varying_009.png
        pi_varying_010.png
        pi_varying_011.png
        pi_varying_012.png
        pi_varying_013.png
        pi_varying_014.png
        pi_varying_015.png
        pi_varying_018.png
        pi_varying_019.png
        pi_varying_020.png
        pi_varying_021.png
        pi_varying_022.png
        pi_varying_023.png
        pi_varying_024.png
        pi_varying_025.png
        pi_varying_026.png
        pi_varying_027.png
        pi_varying_028.png
    FAIL .05
    FAIL_PERCENT .03
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPointInstancer
)

pxr_register_test(testUsdImagingGLPointInstancer_pi_pi_varying
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -camlight -translate 0 -10 -40 -clear 0.2 0.2 0.2 1 -stage pi_pi_varying.usda -times 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 -write pi_pi_varying.png"
    IMAGE_DIFF_COMPARE
        pi_pi_varying_000.png
        pi_pi_varying_001.png
        pi_pi_varying_002.png
        pi_pi_varying_003.png
        pi_pi_varying_004.png
        pi_pi_varying_005.png
        pi_pi_varying_006.png
        pi_pi_varying_007.png
        pi_pi_varying_008.png
        pi_pi_varying_009.png
        pi_pi_varying_010.png
        pi_pi_varying_011.png
        pi_pi_varying_012.png
        pi_pi_varying_013.png
        pi_pi_varying_014.png
        pi_pi_varying_015.png
        pi_pi_varying_016.png
        pi_pi_varying_017.png
    FAIL .05
    FAIL_PERCENT .03
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPointInstancer
)

pxr_register_test(testUsdImagingGLPointInstancer_pi_pi_time
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -camlight -translate 0 -10 -40 -clear 0.2 0.2 0.2 1 -stage pi_pi_time.usda -times 0 1 2 -write pi_pi_time.png"
    IMAGE_DIFF_COMPARE        # Skipping pi_pi_time_002.png because there is a bug where the prototype root transform is not taking into account for Proto3 and Proto4.
        pi_pi_time_000.png    # This bug appears when using the UsdImaging delegate but is fixed in the scene index implementation (using UsdImagingPiPrototypePropagatingSceneIndex).
        pi_pi_time_001.png
    FAIL .05
    FAIL_PERCENT .03
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPointInstancer
)

pxr_register_test(testUsdImagingGLPointInstancer_pi_masking
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -frameAll -lighting -camlight -stage pi_masking.usda -times 0 1 2 3 4 -write pi_masking.png"
    IMAGE_DIFF_COMPARE
        pi_masking_000.png
        pi_masking_001.png
        pi_masking_002.png
        pi_masking_003.png
        pi_masking_004.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPointInstancer
)

pxr_register_test(testUsdImagingGLPointInstancer_pi_bad_schema_input
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -frameAll -lighting -camlight -stage pi_bad_schema_input.usda -write pi_bad_schema_input.png"
    IMAGE_DIFF_COMPARE
        pi_bad_schema_input.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPointInstancer
)

pxr_register_test(testUsdImagingGLPointInstancer_pi_ni_material
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -frameAll -lighting -camlight -stage pi_ni_material.usda -write pi_ni_material.png"
    IMAGE_DIFF_COMPARE
        pi_ni_material.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPointInstancer
)

pxr_register_test(testUsdImagingGLPointInstancer_pi_ni_material2
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -frameAll -lighting -camlight -stage pi_ni_material2.usda -write pi_ni_material2.png"
    IMAGE_DIFF_COMPARE
        pi_ni_material2.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPointInstancer
)

pxr_register_test(testUsdImagingGLPointInstancer_SceneIndex_pi_basic
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -frameAll -lighting -camlight -stage pi_basic.usda -times 1 2 -write pi_basic.png"
    IMAGE_DIFF_COMPARE
        pi_basic_001.png
        pi_basic_002.png
    FAIL .05
    FAIL_PERCENT .03
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPointInstancer_SceneIndex
    ENV
        USDIMAGINGGL_ENGINE_ENABLE_SCENE_INDEX=true
)

pxr_register_test(testUsdImagingGLPointInstancer_SceneIndex_pi
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -frameAll -lighting -camlight -stage pi.usda -write pi.png"
    IMAGE_DIFF_COMPARE
        pi.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPointInstancer_SceneIndex
    ENV
        USDIMAGINGGL_ENGINE_ENABLE_SCENE_INDEX=true
)

pxr_register_test(testUsdImagingGLPointInstancer_SceneIndex_pi_pi
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -frameAll -lighting -camlight -stage pi_pi.usda -write pi_pi.png"
    IMAGE_DIFF_COMPARE
        pi_pi.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPointInstancer_SceneIndex
    ENV
        USDIMAGINGGL_ENGINE_ENABLE_SCENE_INDEX=true
)

pxr_register_test(testUsdImagingGLPointInstancer_SceneIndex_pi_pi_which_rot
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -frameAll -lighting -camlight -stage pi_pi_which_rot.usda -write pi_pi_which_rot.png"
    IMAGE_DIFF_COMPARE
        pi_pi_which_rot.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPointInstancer_SceneIndex
    ENV
        USDIMAGINGGL_ENGINE_ENABLE_SCENE_INDEX=true
)

pxr_register_test(testUsdImagingGLPointInstancer_SceneIndex_pi_pi2
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -frameAll -lighting -camlight -stage pi_pi2.usda -write pi_pi2.png"
    IMAGE_DIFF_COMPARE
        pi_pi2.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPointInstancer_SceneIndex
    ENV
        USDIMAGINGGL_ENGINE_ENABLE_SCENE_INDEX=true
)

pxr_register_test(testUsdImagingGLPointInstancer_SceneIndex_ni_pi
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -frameAll -lighting -camlight -stage ni_pi.usda -write ni_pi.png"
    IMAGE_DIFF_COMPARE
        ni_pi.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPointInstancer_SceneIndex
    ENV
        USDIMAGINGGL_ENGINE_ENABLE_SCENE_INDEX=true
)

pxr_register_test(testUsdImagingGLPointInstancer_SceneIndex_pi_ni
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -frameAll -lighting -camlight -stage pi_ni.usda -write pi_ni.png"
    IMAGE_DIFF_COMPARE
        pi_ni.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPointInstancer_SceneIndex
    ENV
        USDIMAGINGGL_ENGINE_ENABLE_SCENE_INDEX=true
)

pxr_register_test(testUsdImagingGLPointInstancer_SceneIndex_pi_ni2
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -frameAll -lighting -camlight -stage pi_ni2.usda -write pi_ni2.png"
    IMAGE_DIFF_COMPARE
        pi_ni2.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPointInstancer_SceneIndex
    ENV
        USDIMAGINGGL_ENGINE_ENABLE_SCENE_INDEX=true
)

pxr_register_test(testUsdImagingGLPointInstancer_SceneIndex_pi_ni_pi
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -frameAll -lighting -camlight -stage pi_ni_pi.usda -write pi_ni_pi.png"
    IMAGE_DIFF_COMPARE
        pi_ni_pi.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPointInstancer_SceneIndex
    ENV
        USDIMAGINGGL_ENGINE_ENABLE_SCENE_INDEX=true
)

pxr_register_test(testUsdImagingGLPointInstancer_SceneIndex_pi_root_prototype
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -frameAll -lighting -camlight -stage pi_root_prototype.usda -write pi_root_prototype.png"
    IMAGE_DIFF_COMPARE
        pi_root_prototype.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPointInstancer_SceneIndex
    ENV
        USDIMAGINGGL_ENGINE_ENABLE_SCENE_INDEX=true
)

pxr_register_test(testUsdImagingGLPointInstancer_SceneIndex_pi_sibling_prototype
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -frameAll -lighting -camlight -stage pi_sibling_prototype.usda -write pi_sibling_prototype.png"
    IMAGE_DIFF_COMPARE
        pi_sibling_prototype.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPointInstancer_SceneIndex
    ENV
        USDIMAGINGGL_ENGINE_ENABLE_SCENE_INDEX=true
)

pxr_register_test(testUsdImagingGLPointInstancer_SceneIndex_pi_sibling_pi
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -frameAll -lighting -camlight -stage pi_sibling_pi.usda -write pi_sibling_pi.png"
    IMAGE_DIFF_COMPARE
        pi_sibling_pi.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPointInstancer_SceneIndex
    ENV
        USDIMAGINGGL_ENGINE_ENABLE_SCENE_INDEX=true
)

pxr_register_test(testUsdImagingGLPointInstancer_SceneIndex_pi_sibling_pi_over
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -frameAll -lighting -camlight -stage pi_sibling_pi.usda -write pi_sibling_pi_over.png"
    IMAGE_DIFF_COMPARE
        pi_sibling_pi_over.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPointInstancer_SceneIndex
    ENV
        USDIMAGINGGL_ENGINE_ENABLE_SCENE_INDEX=true
)

# UsdImagingPiPrototypePropagtingSceneIndex does not support yet some visibility aspects.
#pxr_register_test(testUsdImagingGLPointInstancer_SceneIndex_pi_varying
#    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -camlight -translate 0 -10 -40 -clear 0.2 0.2 0.2 1 -stage pi_varying.usda -times 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 18 19 20 21 22 23 24 25 26 27 28 29 30 -write pi_varying.png"
#    IMAGE_DIFF_COMPARE
#        pi_varying_000.png
#        pi_varying_001.png
#        pi_varying_002.png
#        pi_varying_003.png
#        pi_varying_004.png
#        pi_varying_005.png
#        pi_varying_006.png
#        pi_varying_007.png
#        pi_varying_008.png
#        pi_varying_009.png
#        pi_varying_010.png
#        pi_varying_011.png
#        pi_varying_012.png
#        pi_varying_013.png
#        pi_varying_014.png
#        pi_varying_015.png
#        pi_varying_018.png
#        pi_varying_019.png
#        pi_varying_020.png
#        pi_varying_021.png
#        pi_varying_022.png
#        pi_varying_023.png
#        pi_varying_024.png
#        pi_varying_025.png
#        pi_varying_026.png
#        pi_varying_027.png
#        pi_varying_028.png
#    FAIL .05
#    FAIL_PERCENT .03
#    PERCEPTUAL
#    EXPECTED_RETURN_CODE 0
#    TESTENV testUsdImagingGLPointInstancer_SceneIndex
#    ENV
#        USDIMAGINGGL_ENGINE_ENABLE_SCENE_INDEX=true
#)

pxr_register_test(testUsdImagingGLPointInstancer_SceneIndex_pi_pi_varying
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -camlight -translate 0 -10 -40 -clear 0.2 0.2 0.2 1 -stage pi_pi_varying.usda -times 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 -write pi_pi_varying.png"
    IMAGE_DIFF_COMPARE
        pi_pi_varying_000.png
        pi_pi_varying_001.png
        pi_pi_varying_002.png
        pi_pi_varying_003.png
        pi_pi_varying_004.png
        pi_pi_varying_005.png
        pi_pi_varying_006.png
        pi_pi_varying_007.png
        pi_pi_varying_008.png
        pi_pi_varying_009.png
        pi_pi_varying_010.png
        pi_pi_varying_011.png
        pi_pi_varying_012.png
        pi_pi_varying_013.png
        pi_pi_varying_014.png
        pi_pi_varying_015.png
        pi_pi_varying_016.png
        pi_pi_varying_017.png
    FAIL .05
    FAIL_PERCENT .03
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPointInstancer_SceneIndex
    ENV
        USDIMAGINGGL_ENGINE_ENABLE_SCENE_INDEX=true
)

pxr_register_test(testUsdImagingGLPointInstancer_pi_pi_varyingf
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -camlight -translate 0 -10 -40 -clear 0.2 0.2 0.2 1 -stage pi_pi_varyingf.usda -times 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 -write pi_pi_varyingf.png"
    IMAGE_DIFF_COMPARE
        pi_pi_varyingf_000.png
        pi_pi_varyingf_001.png
        pi_pi_varyingf_002.png
        pi_pi_varyingf_003.png
        pi_pi_varyingf_004.png
        pi_pi_varyingf_005.png
        pi_pi_varyingf_006.png
        pi_pi_varyingf_007.png
        pi_pi_varyingf_008.png
        pi_pi_varyingf_009.png
        pi_pi_varyingf_010.png
        pi_pi_varyingf_011.png
        pi_pi_varyingf_012.png
        pi_pi_varyingf_013.png
        pi_pi_varyingf_014.png
        pi_pi_varyingf_015.png
        pi_pi_varyingf_016.png
        pi_pi_varyingf_017.png
    FAIL .05
    FAIL_PERCENT .03
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPointInstancer
)

pxr_register_test(testUsdImagingGLPointInstancer_SceneIndex_pi_pi_time
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -camlight -translate 0 -10 -40 -clear 0.2 0.2 0.2 1 -stage pi_pi_time.usda -times 0 1 2 -write pi_pi_time.png"
    IMAGE_DIFF_COMPARE        # Skipping pi_pi_time_002.png because there is a bug where the prototype root transform is not taking into account for Proto3 and Proto4.
        pi_pi_time_000.png
        pi_pi_time_001.png
    FAIL .05
    FAIL_PERCENT .03
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPointInstancer_SceneIndex
    ENV
        USDIMAGINGGL_ENGINE_ENABLE_SCENE_INDEX=true
)

pxr_register_test(testUsdImagingGLPointInstancer_SceneIndex_pi_masking
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -frameAll -lighting -camlight -stage pi_masking.usda -times 0 1 2 3 4 -write pi_masking.png"
    IMAGE_DIFF_COMPARE
        pi_masking_000.png
        pi_masking_001.png
        pi_masking_002.png
        pi_masking_003.png
        pi_masking_004.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPointInstancer_SceneIndex
    ENV
        USDIMAGINGGL_ENGINE_ENABLE_SCENE_INDEX=true
)

# UsdImagingPiPrototypePropagatingSceneIndex does not yet implement
# rejection of bad schemas.
#pxr_register_test(testUsdImagingGLPointInstancer_SceneIndex_pi_bad_schema_input
#    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -frameAll -lighting -camlight -stage pi_bad_schema_input.usda -write pi_bad_schema_input.png"
#    IMAGE_DIFF_COMPARE
#        pi_bad_schema_input.png
#    FAIL 1
#    FAIL_PERCENT 1
#    PERCEPTUAL
#    EXPECTED_RETURN_CODE 0
#    TESTENV testUsdImagingGLPointInstancer_SceneIndex
#    ENV
#        USDIMAGINGGL_ENGINE_ENABLE_SCENE_INDEX=true
#)

pxr_register_test(testUsdImagingGLPointInstancer_SceneIndex_pi_ni_material
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -frameAll -lighting -camlight -stage pi_ni_material.usda -write pi_ni_material.png"
    IMAGE_DIFF_COMPARE
        pi_ni_material.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPointInstancer_SceneIndex
    ENV
        USDIMAGINGGL_ENGINE_ENABLE_SCENE_INDEX=true
)

pxr_register_test(testUsdImagingGLPointInstancer_SceneIndex_pi_ni_material2
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -frameAll -lighting -camlight -stage pi_ni_material2.usda -write pi_ni_material2.png"
    IMAGE_DIFF_COMPARE
        pi_ni_material2.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPointInstancer_SceneIndex
    ENV
        USDIMAGINGGL_ENGINE_ENABLE_SCENE_INDEX=true
)

pxr_register_test(testUsdImagingGLResync
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLResync --stage world.usda --unload /World/Model --write world.png"
    IMAGE_DIFF_COMPARE
        world_0.png
        world_1.png
        world_2.png
    FAIL .05
    FAIL_PERCENT .03
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLResync
)

pxr_register_test(testUsdImagingGLResync_instancer
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLResync --stage instancer.usda --unload /Root/PI/prototypes/proto1 --write instancer.png"
    IMAGE_DIFF_COMPARE
        instancer_0.png
        instancer_1.png
        instancer_2.png
    FAIL .05
    FAIL_PERCENT .03
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLResync
)

pxr_register_test(testUsdImagingGLPurpose
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLPurpose --stage world.usda --purpose /World/Model --write world.png"
    IMAGE_DIFF_COMPARE
        world_0.png
        world_1.png
        world_2.png
        world_3.png
    FAIL .1
    FAIL_PERCENT 10
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPurpose
)

pxr_register_test(testUsdImagingGLPrimvarSharing
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -camlight -frameAll -stage primvar_sharing.usda -times 0 1 2 -write default.png"
    IMAGE_DIFF_COMPARE
        default_000.png
        default_001.png
        default_002.png
    FAIL .05
    FAIL_PERCENT .03
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPrimvarSharing
    ENV
        HDST_ENABLE_SHARED_VERTEX_PRIMVAR=1
)

pxr_register_test(testUsdImagingGLPrimvarSharing_refined
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -complexity 1.1 -lighting -camlight -frameAll -stage primvar_sharing.usda -times 0 1 2 -write refined.png"
    IMAGE_DIFF_COMPARE
        refined_000.png
        refined_001.png
        refined_002.png
    FAIL 0.1
    FAIL_PERCENT 1
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPrimvarSharing
    ENV
        HDST_ENABLE_SHARED_VERTEX_PRIMVAR=1
)

pxr_register_test(testUsdImagingGLPrimvarSharing_colors
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -frameAll -stage primvar_sharing_colors.usda -times 0 1 2 -write default_colors.png"
    IMAGE_DIFF_COMPARE
        default_colors_000.png
        default_colors_001.png
        default_colors_002.png
    FAIL .05
    FAIL_PERCENT .03
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPrimvarSharing
    ENV
        HDST_ENABLE_SHARED_VERTEX_PRIMVAR=1
)

pxr_register_test(testUsdImagingGLPrimvarSharing_refined_colors
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -complexity 1.1 -frameAll -stage primvar_sharing_colors.usda -times 0 1 2 -write refined_colors.png"
    IMAGE_DIFF_COMPARE
        refined_colors_000.png
        refined_colors_001.png
        refined_colors_002.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPrimvarSharing
    ENV
        HDST_ENABLE_SHARED_VERTEX_PRIMVAR=1
)

pxr_register_test(testUsdImagingGLColorOpacityPrimvar
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -translate -5 -2 -25 -times 0 1 -stage color_opacity_primvar.usda -write color_opacity_primvar.png"
    IMAGE_DIFF_COMPARE
        color_opacity_primvar_000.png
        color_opacity_primvar_001.png
    FAIL .05
    FAIL_PERCENT .03
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLColorOpacityPrimvar
)

pxr_register_test(testUsdImagingGLTriangleSubdivisionRuleOsd3
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -shading smooth -frameAll -complexity 1.5 -stage catmark_and_smooth_tris.usda -write testUsdImagingGLTriangleSubdivisionRuleOsd3.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLTriangleSubdivisionRuleOsd3.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLTriangleSubdivisionRuleOsd3
)

pxr_register_test(testUsdImagingGLWireframeOpacity
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -shading wire -frameAll -complexity 1 -stage wireframe_opacity.usda -write testUsdImagingGLWireframeOpacity.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLWireframeOpacity.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLWireframeOpacity
)

pxr_register_test(testUsdImagingGLCards
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -shading smooth -frameAll -stage testCards.usda -write testCards.png"
    IMAGE_DIFF_COMPARE
        testCards.png
    FAIL 0.01
    FAIL_PERCENT 0.1
    WARN 0.02
    WARN_PERCENT 0.05
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLCards
)

pxr_register_test(testUsdImagingGLCards_anim
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -shading smooth -frameAll -stage testAnim.usda -times 1 2 -write testAnim.png"
    IMAGE_DIFF_COMPARE
        testAnim_001.png
        testAnim_002.png
    FAIL 0.01
    FAIL_PERCENT 0.1
    WARN 0.02
    WARN_PERCENT 0.05
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLCards
)

pxr_register_test(testUsdImagingGLCards_bounds
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -unloadedAsBounds -offscreen -lighting -shading smooth -frameAll -stage testUnloadedAsBounds.usda -write testUnloadedAsBounds.png"
    IMAGE_DIFF_COMPARE
        testUnloadedAsBounds.png
    FAIL 0.01
    FAIL_PERCENT 0.1
    WARN 0.02
    WARN_PERCENT 0.05
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLCards
)

pxr_register_test(testUsdImagingGLMeshQuadrangulation
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -shading wireOnSurface -frameAll -stage meshQuadrangulation.usda -write testUsdImagingGLMeshQuadrangulation_default.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLMeshQuadrangulation_default.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLMeshQuadrangulation
    ENV
        HD_ENABLE_FORCE_QUADRANGULATE=1
)

pxr_register_test(testUsdImagingGLMeshQuadrangulation_refined
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -shading wireOnSurface -frameAll -stage meshQuadrangulation.usda -complexity 1.1 -write testUsdImagingGLMeshQuadrangulation_refined.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLMeshQuadrangulation_refined.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLMeshQuadrangulation
    ENV
        HD_ENABLE_FORCE_QUADRANGULATE=1
)

pxr_register_test(testUsdImagingGLSkeleton
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -frameAll -guidesPurpose show -shading wire -stage skeleton.usda -times 1 2 3 -write skeleton.png"
    IMAGE_DIFF_COMPARE
        skeleton_001.png
        skeleton_002.png
        skeleton_003.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLSkeleton
)

pxr_register_test(testUsdImagingGLSkelBlendShapes
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -frameAll -stage blendshapes.usda -times 1 2 3 -write blendshapes.png"
    IMAGE_DIFF_COMPARE
        blendshapes_001.png
        blendshapes_002.png
        blendshapes_003.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLSkelBlendShapes
)

pxr_register_test(testUsdImagingGLNormals
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -shading smooth -frameAll -stage testNormals.usda -write testNormals.png"
    IMAGE_DIFF_COMPARE
        testNormals.png
    FAIL 0.1
    FAIL_PERCENT 1
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLNormals
)

pxr_register_test(testUsdImagingGLPreviewSurface_Fallback
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -shading smooth -frameAll -stage testPreviewSurfaceFallback.usda -write testPreviewSurfaceFallback.png"
    IMAGE_DIFF_COMPARE
        testPreviewSurfaceFallback.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPreviewSurface
    ENV
        HGIGL_ENABLE_BINDLESS_TEXTURE=1
)


pxr_register_test(testUsdImagingGLPreviewSurface_Primvars
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -shading smooth -frameAll -stage testPreviewSurfacePrimvars.usda -write testPreviewSurfacePrimvars.png"
    IMAGE_DIFF_COMPARE
        testPreviewSurfacePrimvars.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPreviewSurface
    ENV
        HGIGL_ENABLE_BINDLESS_TEXTURE=1
)


pxr_register_test(testUsdImagingGLPreviewSurface_PrimvarReaderOpacity
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -shading smooth -frameAll -stage testPreviewSurfacePrimvarReaderOpacity.usda -write testPreviewSurfacePrimvarReaderOpacity.png"
    IMAGE_DIFF_COMPARE
        testPreviewSurfacePrimvarReaderOpacity.png
    FAIL 0.05
    FAIL_PERCENT 0.001
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPreviewSurface
    ENV
        HGIGL_ENABLE_BINDLESS_TEXTURE=1
)


pxr_register_test(testUsdImagingGLPreviewSurface_Specular
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -shading smooth -frameAll -stage testPreviewSurfaceSpecular.usda -write testPreviewSurfaceSpecular.png"
    IMAGE_DIFF_COMPARE
        testPreviewSurfaceSpecular.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPreviewSurface
    ENV
        HGIGL_ENABLE_BINDLESS_TEXTURE=1
)


pxr_register_test(testUsdImagingGLPreviewSurface_MaterialInterface
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -shading smooth -frameAll -stage testPreviewSurfaceMaterialInterface.usda -write testPreviewSurfaceMaterialInterface.png"
    IMAGE_DIFF_COMPARE
        testPreviewSurfaceMaterialInterface.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPreviewSurface
    ENV
        HGIGL_ENABLE_BINDLESS_TEXTURE=1
)


pxr_register_test(testUsdImagingGLPreviewSurface_Texture
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -shading smooth -frameAll -stage testPreviewSurfaceTexture.usda -write testPreviewSurfaceTexture.png"
    IMAGE_DIFF_COMPARE
        testPreviewSurfaceTexture.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPreviewSurface
    ENV
        HGIGL_ENABLE_BINDLESS_TEXTURE=1
)


pxr_register_test(testUsdImagingGLPreviewSurface_Translucent
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -cullStyle back -offscreen -lighting -shading smooth -frameAll -stage testPreviewSurfaceTranslucent.usda -write testPreviewSurfaceTranslucent.png"
    IMAGE_DIFF_COMPARE
        testPreviewSurfaceTranslucent.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPreviewSurface
    ENV
        HGIGL_ENABLE_BINDLESS_TEXTURE=1
)


pxr_register_test(testUsdImagingGLPreviewSurface_Displacement
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -shading smooth -frameAll -complexity 1.4 -stage testPreviewSurfaceDisplacement.usda -write testPreviewSurfaceDisplacement.png"
    IMAGE_DIFF_COMPARE
        testPreviewSurfaceDisplacement.png
    FAIL 1
    FAIL_PERCENT 0.7
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPreviewSurface
    ENV
        HGIGL_ENABLE_BINDLESS_TEXTURE=1
)


pxr_register_test(testUsdImagingGLPreviewSurface_TextureOpacity
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -shading smooth -frameAll -stage testPreviewSurfaceTextureOpacity.usda -write testPreviewSurfaceTextureOpacity.png"
    IMAGE_DIFF_COMPARE
        testPreviewSurfaceTextureOpacity.png
    FAIL 0.05
    FAIL_PERCENT 0.001
    WARN 0.05
    WARN_PERCENT 0.0005
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPreviewSurface
    ENV
        HGIGL_ENABLE_BINDLESS_TEXTURE=1
)


pxr_register_test(testUsdImagingGLPreviewSurface_TextureScaleAndBias
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -shading smooth -frameAll -stage testPreviewSurfaceTextureScaleAndBias.usda -write testPreviewSurfaceTextureScaleAndBias.png"
    IMAGE_DIFF_COMPARE
        testPreviewSurfaceTextureScaleAndBias.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPreviewSurface
    ENV
        HGIGL_ENABLE_BINDLESS_TEXTURE=1
)


pxr_register_test(testUsdImagingGLPreviewSurface_Transform2d
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -shading smooth -frameAll -stage testPreviewSurfaceTransform2d.usda -write testPreviewSurfaceTransform2d.png"
    IMAGE_DIFF_COMPARE
        testPreviewSurfaceTransform2d.png
    FAIL 0.01
    FAIL_PERCENT 3.5
    WARN 0.02
    WARN_PERCENT 0.1
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPreviewSurface
    ENV
        HGIGL_ENABLE_BINDLESS_TEXTURE=1
)


pxr_register_test(testUsdImagingGLPreviewSurface_SourceColorSpace
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -shading smooth -frameAll -stage testPreviewSurfaceSourceColorSpace.usda -write testPreviewSurfaceSourceColorSpace.png"
    IMAGE_DIFF_COMPARE
        testPreviewSurfaceSourceColorSpace.png
    FAIL 0.005
    FAIL_PERCENT 0.001
    WARN 0.005
    WARN_PERCENT 0.0005
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPreviewSurface
    ENV
        HGIGL_ENABLE_BINDLESS_TEXTURE=1
)


pxr_register_test(testUsdImagingGLPreviewSurface_NormalMap
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -shading smooth -frameAll -stage testPreviewSurfaceNormalMap.usda -write testPreviewSurfaceNormalMap.png"
    IMAGE_DIFF_COMPARE
        testPreviewSurfaceNormalMap.png
    FAIL 0.01
    FAIL_PERCENT 0.5
    WARN 0.1
    WARN_PERCENT 0.0005
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPreviewSurface
    ENV
        HGIGL_ENABLE_BINDLESS_TEXTURE=1
)


pxr_register_test(testUsdImagingGLPreviewSurface_OpacityThreshold
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -shading smooth -frameAll -stage testPreviewSurfaceOpacityThreshold.usda -write testPreviewSurfaceOpacityThreshold.png"
    IMAGE_DIFF_COMPARE
        testPreviewSurfaceOpacityThreshold.png
    FAIL 0.005
    FAIL_PERCENT 0.05
    WARN 0.005
    WARN_PERCENT 0.03
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPreviewSurface
    ENV
        HGIGL_ENABLE_BINDLESS_TEXTURE=1
)

pxr_register_test(testUsdImagingGLPreviewSurface_Animated
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -times 1 2 3 4 -shading smooth -frameAll -stage testPreviewSurfaceAnimated.usda -write testPreviewSurfaceAnimated.png"
    IMAGE_DIFF_COMPARE
        testPreviewSurfaceAnimated_001.png
        testPreviewSurfaceAnimated_002.png
        testPreviewSurfaceAnimated_003.png
        testPreviewSurfaceAnimated_004.png
    FAIL .05
    FAIL_PERCENT .03
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPreviewSurface
    ENV
        HGIGL_ENABLE_BINDLESS_TEXTURE=1
)

pxr_register_test(testUsdImagingGLPreviewSurface_FallbackPrimvar
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -shading smooth -frameAll -stage testPreviewSurfaceFallbackPrimvar.usda -write testPreviewSurfaceFallbackPrimvar.png"
    IMAGE_DIFF_COMPARE
        testPreviewSurfaceFallbackPrimvar.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPreviewSurface
    ENV
        HGIGL_ENABLE_BINDLESS_TEXTURE=1
)

pxr_register_test(testUsdImagingGLPreviewSurface_FallbackTexture
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -shading smooth -frameAll -stage testPreviewSurfaceFallbackTexture.usda -write testPreviewSurfaceFallbackTexture.png"
    IMAGE_DIFF_COMPARE
        testPreviewSurfaceFallbackTexture.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPreviewSurface
    ENV
        HGIGL_ENABLE_BINDLESS_TEXTURE=1
)

pxr_register_test(testUsdImagingGLPreviewSurface_PrimvarFallback
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -shading smooth -frameAll -stage testPreviewSurfacePrimvarFallback.usda -write testPreviewSurfacePrimvarFallback.png"
    IMAGE_DIFF_COMPARE
        testPreviewSurfacePrimvarFallback.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPreviewSurface
    ENV
        HGIGL_ENABLE_BINDLESS_TEXTURE=1
)

pxr_register_test(testUsdImagingGLPreviewSurface_TextureFallback
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -shading smooth -frameAll -stage testPreviewSurfaceTextureFallback.usda -write testPreviewSurfaceTextureFallback.png"
    IMAGE_DIFF_COMPARE
        testPreviewSurfaceTextureFallback.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPreviewSurface
    ENV
        HGIGL_ENABLE_BINDLESS_TEXTURE=1
)

pxr_register_test(testUsdImagingGLPreviewSurface_TextureNoFile
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -shading smooth -frameAll -stage testPreviewSurfaceTextureNoFile.usda -write testPreviewSurfaceTextureNoFile.png"
    IMAGE_DIFF_COMPARE
        testPreviewSurfaceTextureNoFile.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPreviewSurface
    ENV
        HGIGL_ENABLE_BINDLESS_TEXTURE=1
)

pxr_register_test(testUsdImagingGLPreviewSurface_TextureWithToken
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -shading smooth -frameAll -stage testPreviewSurfaceTextureWithToken.usda -write testPreviewSurfaceTextureWithToken.png"
    IMAGE_DIFF_COMPARE
        testPreviewSurfaceTextureWithToken.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPreviewSurface
    ENV
        HGIGL_ENABLE_BINDLESS_TEXTURE=1
)

pxr_register_test(testUsdImagingGLPreviewSurfaceNonBindless_Fallback
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -shading smooth -frameAll -stage testPreviewSurfaceFallback.usda -write testPreviewSurfaceFallback.png"
    IMAGE_DIFF_COMPARE
        testPreviewSurfaceFallback.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPreviewSurface
    ENV
        HGIGL_ENABLE_BINDLESS_TEXTURE=0
)


pxr_register_test(testUsdImagingGLPreviewSurfaceNonBindless_Primvars
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -shading smooth -frameAll -stage testPreviewSurfacePrimvars.usda -write testPreviewSurfacePrimvars.png"
    IMAGE_DIFF_COMPARE
        testPreviewSurfacePrimvars.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPreviewSurface
    ENV
        HGIGL_ENABLE_BINDLESS_TEXTURE=0
)


pxr_register_test(testUsdImagingGLPreviewSurfaceNonBindless_Specular
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -shading smooth -frameAll -stage testPreviewSurfaceSpecular.usda -write testPreviewSurfaceSpecular.png"
    IMAGE_DIFF_COMPARE
        testPreviewSurfaceSpecular.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPreviewSurface
    ENV
        HGIGL_ENABLE_BINDLESS_TEXTURE=0
)


pxr_register_test(testUsdImagingGLPreviewSurfaceNonBindless_MaterialInterface
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -shading smooth -frameAll -stage testPreviewSurfaceMaterialInterface.usda -write testPreviewSurfaceMaterialInterface.png"
    IMAGE_DIFF_COMPARE
        testPreviewSurfaceMaterialInterface.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPreviewSurface
    ENV
        HGIGL_ENABLE_BINDLESS_TEXTURE=0
)


pxr_register_test(testUsdImagingGLPreviewSurfaceNonBindless_Texture
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -shading smooth -frameAll -stage testPreviewSurfaceTexture.usda -write testPreviewSurfaceTexture.png"
    IMAGE_DIFF_COMPARE
        testPreviewSurfaceTexture.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPreviewSurface
    ENV
        HGIGL_ENABLE_BINDLESS_TEXTURE=0
)


pxr_register_test(testUsdImagingGLPreviewSurfaceNonBindless_Translucent
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -cullStyle back -offscreen -lighting -shading smooth -frameAll -stage testPreviewSurfaceTranslucent.usda -write testPreviewSurfaceTranslucent.png"
    IMAGE_DIFF_COMPARE
        testPreviewSurfaceTranslucent.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPreviewSurface
    ENV
        HGIGL_ENABLE_BINDLESS_TEXTURE=0
)


pxr_register_test(testUsdImagingGLPreviewSurfaceNonBindless_Displacement
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -shading smooth -frameAll -complexity 1.4 -stage testPreviewSurfaceDisplacement.usda -write testPreviewSurfaceDisplacement.png"
    IMAGE_DIFF_COMPARE
        testPreviewSurfaceDisplacement.png
    FAIL 1
    FAIL_PERCENT 0.7
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPreviewSurface
    ENV
        HGIGL_ENABLE_BINDLESS_TEXTURE=0
)


pxr_register_test(testUsdImagingGLPreviewSurfaceNonBindless_TextureOpacity
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -shading smooth -frameAll -stage testPreviewSurfaceTextureOpacity.usda -write testPreviewSurfaceTextureOpacity.png"
    IMAGE_DIFF_COMPARE
        testPreviewSurfaceTextureOpacity.png
    FAIL 0.05
    FAIL_PERCENT 0.001
    WARN 0.05
    WARN_PERCENT 0.0005
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPreviewSurface
    ENV
        HGIGL_ENABLE_BINDLESS_TEXTURE=0
)


pxr_register_test(testUsdImagingGLPreviewSurfaceNonBindless_TextureScaleAndBias
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -shading smooth -frameAll -stage testPreviewSurfaceTextureScaleAndBias.usda -write testPreviewSurfaceTextureScaleAndBias.png"
    IMAGE_DIFF_COMPARE
        testPreviewSurfaceTextureScaleAndBias.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPreviewSurface
    ENV
        HGIGL_ENABLE_BINDLESS_TEXTURE=0
)


pxr_register_test(testUsdImagingGLPreviewSurfaceNonBindless_Transform2d
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -shading smooth -frameAll -stage testPreviewSurfaceTransform2d.usda -write testPreviewSurfaceTransform2d.png"
    IMAGE_DIFF_COMPARE
        testPreviewSurfaceTransform2d.png
    FAIL 0.01
    FAIL_PERCENT 3.5
    WARN 0.02
    WARN_PERCENT 0.1
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPreviewSurface
    ENV
        HGIGL_ENABLE_BINDLESS_TEXTURE=0
)


pxr_register_test(testUsdImagingGLPreviewSurfaceNonBindless_SourceColorSpace
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -shading smooth -frameAll -stage testPreviewSurfaceSourceColorSpace.usda -write testPreviewSurfaceSourceColorSpace.png"
    IMAGE_DIFF_COMPARE
        testPreviewSurfaceSourceColorSpace.png
    FAIL 0.005
    FAIL_PERCENT 0.001
    WARN 0.005
    WARN_PERCENT 0.0005
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPreviewSurface
    ENV
        HGIGL_ENABLE_BINDLESS_TEXTURE=0
)


pxr_register_test(testUsdImagingGLPreviewSurfaceNonBindless_NormalMap
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -shading smooth -frameAll -stage testPreviewSurfaceNormalMap.usda -write testPreviewSurfaceNormalMap.png"
    IMAGE_DIFF_COMPARE
        testPreviewSurfaceNormalMap.png
    FAIL 0.01
    FAIL_PERCENT 0.5
    WARN 0.1
    WARN_PERCENT 0.0005
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPreviewSurface
    ENV
        HGIGL_ENABLE_BINDLESS_TEXTURE=0
)


pxr_register_test(testUsdImagingGLPreviewSurfaceNonBindless_OpacityThreshold
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -shading smooth -frameAll -stage testPreviewSurfaceOpacityThreshold.usda -write testPreviewSurfaceOpacityThreshold.png"
    IMAGE_DIFF_COMPARE
        testPreviewSurfaceOpacityThreshold.png
    FAIL 0.005
    FAIL_PERCENT 0.05
    WARN 0.005
    WARN_PERCENT 0.03
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPreviewSurface
    ENV
        HGIGL_ENABLE_BINDLESS_TEXTURE=0
)


pxr_register_test(testUsdImagingGLPreviewSurfaceNonBindless_Animated
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -times 1 2 3 4 -shading smooth -frameAll -stage testPreviewSurfaceAnimated.usda -write testPreviewSurfaceAnimated.png"
    IMAGE_DIFF_COMPARE
        testPreviewSurfaceAnimated_001.png
        testPreviewSurfaceAnimated_002.png
        testPreviewSurfaceAnimated_003.png
        testPreviewSurfaceAnimated_004.png
    FAIL .05
    FAIL_PERCENT .03
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPreviewSurface
    ENV
        HGIGL_ENABLE_BINDLESS_TEXTURE=0
)


pxr_register_test(testUsdImagingGLPreviewSurfaceNonBindless_FallbackPrimvar
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -shading smooth -frameAll -stage testPreviewSurfaceFallbackPrimvar.usda -write testPreviewSurfaceFallbackPrimvar.png"
    IMAGE_DIFF_COMPARE
        testPreviewSurfaceFallbackPrimvar.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPreviewSurface
    ENV
        HGIGL_ENABLE_BINDLESS_TEXTURE=0
)


pxr_register_test(testUsdImagingGLPreviewSurfaceNonBindless_FallbackTexture
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -shading smooth -frameAll -stage testPreviewSurfaceFallbackTexture.usda -write testPreviewSurfaceFallbackTexture.png"
    IMAGE_DIFF_COMPARE
        testPreviewSurfaceFallbackTexture.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPreviewSurface
    ENV
        HGIGL_ENABLE_BINDLESS_TEXTURE=0
)


pxr_register_test(testUsdImagingGLPreviewSurfaceNonBindless_PrimvarFallback
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -shading smooth -frameAll -stage testPreviewSurfacePrimvarFallback.usda -write testPreviewSurfacePrimvarFallback.png"
    IMAGE_DIFF_COMPARE
        testPreviewSurfacePrimvarFallback.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPreviewSurface
    ENV
        HGIGL_ENABLE_BINDLESS_TEXTURE=0
)


pxr_register_test(testUsdImagingGLPreviewSurfaceNonBindless_TextureFallback
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -shading smooth -frameAll -stage testPreviewSurfaceTextureFallback.usda -write testPreviewSurfaceTextureFallback.png"
    IMAGE_DIFF_COMPARE
        testPreviewSurfaceTextureFallback.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPreviewSurface
    ENV
        HGIGL_ENABLE_BINDLESS_TEXTURE=0
)


pxr_register_test(testUsdImagingGLPreviewSurfaceNonBindless_TextureNoFile
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -shading smooth -frameAll -stage testPreviewSurfaceTextureNoFile.usda -write testPreviewSurfaceTextureNoFile.png"
    IMAGE_DIFF_COMPARE
        testPreviewSurfaceTextureNoFile.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPreviewSurface
    ENV
        HGIGL_ENABLE_BINDLESS_TEXTURE=0
)


pxr_register_test(testUsdImagingGLPreviewSurfaceNonBindless_TextureWithToken
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -shading smooth -frameAll -stage testPreviewSurfaceTextureWithToken.usda -write testPreviewSurfaceTextureWithToken.png"
    IMAGE_DIFF_COMPARE
        testPreviewSurfaceTextureWithToken.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPreviewSurface
    ENV
        HGIGL_ENABLE_BINDLESS_TEXTURE=0
)

pxr_register_test(testUsdImagingGLPremultiplyAlpha
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -shading smooth -frameAll -stage testPremultiplyAlpha.usda -write testPremultiplyAlpha.png"
    IMAGE_DIFF_COMPARE
        testPremultiplyAlpha.png
    FAIL 0.01
    FAIL_PERCENT 0.75
    WARN 0.01
    WARN_PERCENT 0.72
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPremultiplyAlpha
)

pxr_register_test(testUsdImagingGLPremultiplyAlpha_udims
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -shading smooth -frameAll -stage testPremultiplyAlphaUdims.usda -write testPremultiplyAlphaUdims.png"
    IMAGE_DIFF_COMPARE
        testPremultiplyAlphaUdims.png
    FAIL 0.01
    FAIL_PERCENT 0.75
    WARN 0.01
    WARN_PERCENT 0.66
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPremultiplyAlpha
)

pxr_register_test(testUsdImagingGLInstancingWithMaterials_ni_2instances
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -camlight -shading wireOnSurface -frameAll -stage ni_2instances.usda -write testUsdImagingGLInstancingWithMaterials_ni_2instances.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLInstancingWithMaterials_ni_2instances.png
    FAIL 0.01
    FAIL_PERCENT 0.005
    WARN 0.02
    WARN_PERCENT 0.0025
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLInstancingWithMaterials
)


pxr_register_test(testUsdImagingGLInstancingWithMaterials_ni_2instances_per_instance_overrides_noshading
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -camlight -shading wireOnSurface -frameAll -stage ni_2instances_per_instance_overrides_noshading.usda -write testUsdImagingGLInstancingWithMaterials_ni_2instances_per_instance_overrides_noshading.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLInstancingWithMaterials_ni_2instances_per_instance_overrides_noshading.png
    FAIL 0.01
    FAIL_PERCENT 0.005
    WARN 0.02
    WARN_PERCENT 0.0025
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLInstancingWithMaterials
)


pxr_register_test(testUsdImagingGLInstancingWithMaterials_ni_2instances_shaded
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -camlight -shading wireOnSurface -frameAll -stage ni_2instances_shaded.usda -write testUsdImagingGLInstancingWithMaterials_ni_2instances_shaded.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLInstancingWithMaterials_ni_2instances_shaded.png
    FAIL 0.01
    FAIL_PERCENT 0.005
    WARN 0.02
    WARN_PERCENT 0.0025
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLInstancingWithMaterials
)


pxr_register_test(testUsdImagingGLInstancingWithMaterials_ni_2instances_shaded_overrides
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -camlight -shading wireOnSurface -frameAll -stage ni_2instances_shaded_overrides.usda -write testUsdImagingGLInstancingWithMaterials_ni_2instances_shaded_overrides.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLInstancingWithMaterials_ni_2instances_shaded_overrides.png
    FAIL 0.01
    FAIL_PERCENT 0.005
    WARN 0.02
    WARN_PERCENT 0.0025
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLInstancingWithMaterials
)

pxr_register_test(testUsdImagingGLInstancingWithMaterials_ni_ni_material_outside_prototype
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -sceneLights -shading wireOnSurface -camera /main_cam -stage ni_ni_material_outside_prototype.usda -write testUsdImagingGLInstancingWithMaterials_ni_ni_material_outside_prototype.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLInstancingWithMaterials_ni_ni_material_outside_prototype.png
    FAIL 0.01
    FAIL_PERCENT 0.005
    WARN 0.02
    WARN_PERCENT 0.0025
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLInstancingWithMaterials
)

pxr_register_test(testUsdImagingGLInstancingWithMaterials_SceneIndex_ni_2instances
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -camlight -shading wireOnSurface -frameAll -stage ni_2instances.usda -write testUsdImagingGLInstancingWithMaterials_ni_2instances.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLInstancingWithMaterials_ni_2instances.png
    FAIL 0.01
    FAIL_PERCENT 0.005
    WARN 0.02
    WARN_PERCENT 0.0025
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLInstancingWithMaterials_SceneIndex
    ENV
        USDIMAGINGGL_ENGINE_ENABLE_SCENE_INDEX=true
)


pxr_register_test(testUsdImagingGLInstancingWithMaterials_SceneIndex_ni_2instances_per_instance_overrides_noshading
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -camlight -shading wireOnSurface -frameAll -stage ni_2instances_per_instance_overrides_noshading.usda -write testUsdImagingGLInstancingWithMaterials_ni_2instances_per_instance_overrides_noshading.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLInstancingWithMaterials_ni_2instances_per_instance_overrides_noshading.png
    FAIL 0.01
    FAIL_PERCENT 0.005
    WARN 0.02
    WARN_PERCENT 0.0025
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLInstancingWithMaterials_SceneIndex
    ENV
        USDIMAGINGGL_ENGINE_ENABLE_SCENE_INDEX=true
)


pxr_register_test(testUsdImagingGLInstancingWithMaterials_SceneIndex_ni_2instances_shaded
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -camlight -shading wireOnSurface -frameAll -stage ni_2instances_shaded.usda -write testUsdImagingGLInstancingWithMaterials_ni_2instances_shaded.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLInstancingWithMaterials_ni_2instances_shaded.png
    FAIL 0.01
    FAIL_PERCENT 0.005
    WARN 0.02
    WARN_PERCENT 0.0025
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLInstancingWithMaterials_SceneIndex
    ENV
        USDIMAGINGGL_ENGINE_ENABLE_SCENE_INDEX=true
)


pxr_register_test(testUsdImagingGLInstancingWithMaterials_SceneIndex_ni_2instances_shaded_overrides
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -camlight -shading wireOnSurface -frameAll -stage ni_2instances_shaded_overrides.usda -write testUsdImagingGLInstancingWithMaterials_ni_2instances_shaded_overrides.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLInstancingWithMaterials_ni_2instances_shaded_overrides.png
    FAIL 0.01
    FAIL_PERCENT 0.005
    WARN 0.02
    WARN_PERCENT 0.0025
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLInstancingWithMaterials_SceneIndex
    ENV
        USDIMAGINGGL_ENGINE_ENABLE_SCENE_INDEX=true
)

pxr_register_test(testUsdImagingGLInstancingWithMaterials_SceneIndex_ni_ni_material_outside_prototype
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -sceneLights -shading wireOnSurface -camera /main_cam -stage ni_ni_material_outside_prototype.usda -write testUsdImagingGLInstancingWithMaterials_ni_ni_material_outside_prototype.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLInstancingWithMaterials_ni_ni_material_outside_prototype.png
    FAIL 0.01
    FAIL_PERCENT 0.005
    WARN 0.02
    WARN_PERCENT 0.0025
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLInstancingWithMaterials_SceneIndex
    ENV
        USDIMAGINGGL_ENGINE_ENABLE_SCENE_INDEX=true
)

pxr_register_test(testUsdImagingGLInstancingWithMaterials_SceneIndex_ni_ni_material_binding_resolution
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -sceneLights -shading wireOnSurface -frameAll -stage ni_ni_material_binding_resolution.usda -write testUsdImagingGLInstancingWithMaterials_ni_ni_material_binding_resolution.png"
    IMAGE_DIFF_COMPARE
    testUsdImagingGLInstancingWithMaterials_ni_ni_material_binding_resolution.png
    FAIL 0.01
    FAIL_PERCENT 0.005
    WARN 0.02
    WARN_PERCENT 0.0025
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLInstancingWithMaterials_SceneIndex
    ENV
        USDIMAGINGGL_ENGINE_ENABLE_SCENE_INDEX=true
)

pxr_register_test(testUsdImagingGLFaceVarying_Uniform_Low
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -complexity 1.1 -offscreen -lighting -camlight -shading wireOnSurface -frameAll -stage testUsdImagingGLFaceVarying.usda -write testUsdImagingGLFaceVarying_Uniform_Low.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLFaceVarying_Uniform_Low.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLFaceVarying
)


pxr_register_test(testUsdImagingGLFaceVarying_Uniform_Medium
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -complexity 1.2 -offscreen -lighting -camlight -shading wireOnSurface -frameAll -stage testUsdImagingGLFaceVarying.usda -write testUsdImagingGLFaceVarying_Uniform_Medium.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLFaceVarying_Uniform_Medium.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLFaceVarying
)


pxr_register_test(testUsdImagingGLFaceVaryingLinearInterp_Uniform_Low
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -complexity 1.1 -offscreen -lighting -camlight -shading wireOnSurface -frameAll -stage testUsdImagingGLFaceVaryingLinearInterp.usda -write testUsdImagingGLFaceVaryingLinearInterp_Uniform_Low.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLFaceVaryingLinearInterp_Uniform_Low.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLFaceVarying
)


pxr_register_test(testUsdImagingGLFaceVaryingLinearInterp_Uniform_Medium
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -complexity 1.2 -offscreen -lighting -camlight -shading wireOnSurface -frameAll -stage testUsdImagingGLFaceVaryingLinearInterp.usda -write testUsdImagingGLFaceVaryingLinearInterp_Uniform_Medium.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLFaceVaryingLinearInterp_Uniform_Medium.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLFaceVarying
)


pxr_register_test(testUsdImagingGLFaceVaryingLeftHanded_Uniform_None
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -complexity 1.0 -offscreen -lighting -camlight -shading wireOnSurface -frameAll -stage testUsdImagingGLFaceVaryingLeftHanded.usda -write testUsdImagingGLFaceVaryingLeftHanded_Uniform_None.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLFaceVaryingLeftHanded_Uniform_None.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLFaceVarying
)


pxr_register_test(testUsdImagingGLFaceVaryingLeftHanded_Uniform_Low
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -complexity 1.1 -offscreen -lighting -camlight -shading wireOnSurface -frameAll -stage testUsdImagingGLFaceVaryingLeftHanded.usda -write testUsdImagingGLFaceVaryingLeftHanded_Uniform_Low.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLFaceVaryingLeftHanded_Uniform_Low.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLFaceVarying
)

pxr_register_test(testUsdImagingGLFaceVarying_Adaptive_Low
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -complexity 1.1 -offscreen -lighting -camlight -shading wireOnSurface -frameAll -stage testUsdImagingGLFaceVarying.usda -write testUsdImagingGLFaceVarying_Adaptive_Low.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLFaceVarying_Adaptive_Low.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLFaceVarying
    ENV
        HD_ENABLE_OPENSUBDIV3_ADAPTIVE=1
)


pxr_register_test(testUsdImagingGLFaceVarying_Adaptive_Medium
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -complexity 1.2 -offscreen -lighting -camlight -shading wireOnSurface -frameAll -stage testUsdImagingGLFaceVarying.usda -write testUsdImagingGLFaceVarying_Adaptive_Medium.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLFaceVarying_Adaptive_Medium.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLFaceVarying
    ENV
        HD_ENABLE_OPENSUBDIV3_ADAPTIVE=1
)


pxr_register_test(testUsdImagingGLFaceVaryingLinearInterp_Adaptive_Low
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -complexity 1.1 -offscreen -lighting -camlight -shading wireOnSurface -frameAll -stage testUsdImagingGLFaceVaryingLinearInterp.usda -write testUsdImagingGLFaceVaryingLinearInterp_Adaptive_Low.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLFaceVaryingLinearInterp_Adaptive_Low.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLFaceVarying
    ENV
        HD_ENABLE_OPENSUBDIV3_ADAPTIVE=1
)


pxr_register_test(testUsdImagingGLFaceVaryingLinearInterp_Adaptive_Medium
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -complexity 1.2 -offscreen -lighting -camlight -shading wireOnSurface -frameAll -stage testUsdImagingGLFaceVaryingLinearInterp.usda -write testUsdImagingGLFaceVaryingLinearInterp_Adaptive_Medium.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLFaceVaryingLinearInterp_Adaptive_Medium.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLFaceVarying
    ENV
        HD_ENABLE_OPENSUBDIV3_ADAPTIVE=1
)


pxr_register_test(testUsdImagingGLFaceVaryingLeftHanded_Adaptive_Low
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -complexity 1.1 -offscreen -lighting -camlight -shading wireOnSurface -frameAll -stage testUsdImagingGLFaceVaryingLeftHanded.usda -write testUsdImagingGLFaceVaryingLeftHanded_Adaptive_Low.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLFaceVaryingLeftHanded_Adaptive_Low.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLFaceVarying
    ENV
        HD_ENABLE_OPENSUBDIV3_ADAPTIVE=1
)

pxr_register_test(testUsdImagingGLShadingPrimvars
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -shading smooth -frameAll -stage testPrimvarNameHandlingInGL.usda -write testUsdImagingGLShadingPrimvars.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLShadingPrimvars.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLShadingPrimvars
    ENV
        HD_ENABLE_OPENSUBDIV3_ADAPTIVE=1
)

# The following tests are disabled since the stages are unable to load any .tx files
# In theory these image files should be able to load through the hioOiio plugin but I have not
# been able to get that to work. I did try to convert these images through OIIO's iconvert tool
# but it also fails trying to open the .tx files
#pxr_register_test(testUsdImagingGLTextureWrap
#    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -shading smooth -frameAll -stage textureWrap.usda -write textureWrap.png"
#    IMAGE_DIFF_COMPARE
#        textureWrap.png
#    FAIL 1
#    FAIL_PERCENT 1
#    PERCEPTUAL
#    EXPECTED_RETURN_CODE 0
#    TESTENV testUsdImagingGLTextureWrap
#    ENV
#        HGIGL_ENABLE_BINDLESS_TEXTURE=1
#)
#
#pxr_register_test(testUsdImagingGLTextureWrapNonBindless
#    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -shading smooth -frameAll -stage textureWrap.usda -write textureWrap.png"
#    IMAGE_DIFF_COMPARE
#        textureWrap.png
#    FAIL 1
#    FAIL_PERCENT 1
#    PERCEPTUAL
#    EXPECTED_RETURN_CODE 0
#    TESTENV testUsdImagingGLTextureWrap
#    ENV
#        HGIGL_ENABLE_BINDLESS_TEXTURE=0
#)

pxr_register_test(testUsdImagingGLUsdLux
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -shading smooth -frameAll -stage usdLux.usda -write usdLux.png"
    IMAGE_DIFF_COMPARE
        usdLux.png
    FAIL 0.1
    FAIL_PERCENT 1
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLUsdLux
)

# TODO: Requires create_symlinks.py to run properly
#pxr_register_test(testUsdImagingGLUsdUdims_Symlinked
#    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -shading smooth -frameAll -stage usdUdimsSymlinked.usda -write usdUdimsSymlinked.png"
#    IMAGE_DIFF_COMPARE
#        usdUdimsSymlinked.png
#    FAIL_PERCENT 1
#    PERCEPTUAL
#    EXPECTED_RETURN_CODE 0
#    TESTENV testUsdImagingGLUsdUdims
#    ENV
#        HGIGL_ENABLE_BINDLESS_TEXTURE=1
#)

pxr_register_test(testUsdImagingGLUsdUdims
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -shading smooth -frameAll -stage usdUdims.usda -write usdUdims.png"
    IMAGE_DIFF_COMPARE
        usdUdims.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLUsdUdims
    ENV
        HGIGL_ENABLE_BINDLESS_TEXTURE=1
)

pxr_register_test(testUsdImagingGLUsdUdims_Usdz
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -shading smooth -frameAll -stage usdUdims.usdz -write usdUdimsUsdz.png"
    IMAGE_DIFF_COMPARE
        usdUdimsUsdz.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLUsdUdims
    ENV
        HGIGL_ENABLE_BINDLESS_TEXTURE=1
)

pxr_register_test(testUsdImagingGLUsdUdims_ScaleAndBias
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -shading smooth -frameAll -stage usdUdimsScaleAndBias.usda -write usdUdimsScaleAndBias.png"
    IMAGE_DIFF_COMPARE
        usdUdimsScaleAndBias.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLUsdUdims
    ENV
        HGIGL_ENABLE_BINDLESS_TEXTURE=1
)

pxr_register_test(testUsdImagingGLUsdUdims_TextureFallback
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -shading smooth -frameAll -stage usdUdimsTextureFallback.usda -write usdUdimsTextureFallback.png"
    IMAGE_DIFF_COMPARE
        usdUdimsTextureFallback.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLUsdUdims
    ENV
        HGIGL_ENABLE_BINDLESS_TEXTURE=1
)

# TODO: Requires create_symlinks.py to run properly
#pxr_register_test(testUsdImagingGLUsdUdimsNonBindless_Symlinked
#    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -shading smooth -frameAll -stage usdUdimsSymlinked.usda -write usdUdimsSymlinked.png"
#    IMAGE_DIFF_COMPARE
#        usdUdimsSymlinked.png
#    FAIL_PERCENT 1
#    PERCEPTUAL
#    EXPECTED_RETURN_CODE 0
#    TESTENV testUsdImagingGLUsdUdimsNonBindless
#    ENV
#        HGIGL_ENABLE_BINDLESS_TEXTURE=0
#)

pxr_register_test(testUsdImagingGLUsdUdimsNonBindless
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -shading smooth -frameAll -stage usdUdims.usda -write usdUdims.png"
    IMAGE_DIFF_COMPARE
        usdUdims.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLUsdUdims
    ENV
        HGIGL_ENABLE_BINDLESS_TEXTURE=0
)

pxr_register_test(testUsdImagingGLUsdUdimsNonBindless_ScaleAndBias
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -shading smooth -frameAll -stage usdUdimsScaleAndBias.usda -write usdUdimsScaleAndBias.png"
    IMAGE_DIFF_COMPARE
        usdUdimsScaleAndBias.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLUsdUdims
    ENV
        HGIGL_ENABLE_BINDLESS_TEXTURE=0
)

pxr_register_test(testUsdImagingGLUsdUdimsNonBindless_TextureFallback
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -shading smooth -frameAll -stage usdUdimsTextureFallback.usda -write usdUdimsTextureFallback.png"
    IMAGE_DIFF_COMPARE
        usdUdimsTextureFallback.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLUsdUdims
    ENV
        HGIGL_ENABLE_BINDLESS_TEXTURE=0
)

pxr_register_test(testUsdImagingGLUsdSkelExtCompCPU_arm_baked
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -frameAll -lighting -camlight -shading wireOnSurface -stage arm_baked.usda -times 1 3 7 -write usdSkel_arm_baked.png"
    IMAGE_DIFF_COMPARE
        usdSkel_arm_baked_001.png
        usdSkel_arm_baked_003.png
        usdSkel_arm_baked_007.png
    FAIL 0.1
    FAIL_PERCENT 1
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLUsdSkel
    ENV
        USDSKELIMAGING_FORCE_CPU_COMPUTE=1
)

pxr_register_test(testUsdImagingGLUsdSkelExtCompCPU_arm
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -frameAll -lighting -camlight -shading wireOnSurface -stage arm.usda -times 1 3 7 -write usdSkel_arm_extComp_CPU.png"
    IMAGE_DIFF_COMPARE
        usdSkel_arm_extComp_CPU_001.png
        usdSkel_arm_extComp_CPU_003.png
        usdSkel_arm_extComp_CPU_007.png
    FAIL 0.1
    FAIL_PERCENT 1
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLUsdSkel
    ENV
        USDSKELIMAGING_FORCE_CPU_COMPUTE=1
)

pxr_register_test(testUsdImagingGLUsdSkelExtCompCPU_twoBends
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -frameAll -lighting -camlight -shading wireOnSurface -stage twoBends.usda -write usdSkel_twoBends_extComp_CPU.png"
    IMAGE_DIFF_COMPARE
        usdSkel_twoBends_extComp_CPU.png
    FAIL 0.1
    FAIL_PERCENT 1
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLUsdSkel
    ENV
        USDSKELIMAGING_FORCE_CPU_COMPUTE=1
)

pxr_register_test(testUsdImagingGLUsdSkelExtCompGPU_arm_baked
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -frameAll -lighting -camlight -shading wireOnSurface -stage arm_baked.usda -times 1 3 7 -write usdSkel_arm_baked.png"
    IMAGE_DIFF_COMPARE
        usdSkel_arm_baked_001.png
        usdSkel_arm_baked_003.png
        usdSkel_arm_baked_007.png
    FAIL 0.1
    FAIL_PERCENT 1
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLUsdSkel
    ENV
        USDSKELIMAGING_FORCE_CPU_COMPUTE=0
)

pxr_register_test(testUsdImagingGLUsdSkelExtCompGPU_arm
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -frameAll -lighting -camlight -shading wireOnSurface -stage arm.usda -times 1 3 7 -write usdSkel_arm_extComp_GPU.png"
    IMAGE_DIFF_COMPARE
        usdSkel_arm_extComp_GPU_001.png
        usdSkel_arm_extComp_GPU_003.png
        usdSkel_arm_extComp_GPU_007.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLUsdSkel
    ENV
        USDSKELIMAGING_FORCE_CPU_COMPUTE=0
)

pxr_register_test(testUsdImagingGLUsdSkelExtCompGPU_twoBends
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -frameAll -lighting -camlight -shading wireOnSurface -stage twoBends.usda -write usdSkel_twoBends_extComp_GPU.png"
    IMAGE_DIFF_COMPARE
        usdSkel_twoBends_extComp_GPU.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLUsdSkel
    ENV
        USDSKELIMAGING_FORCE_CPU_COMPUTE=0
)

pxr_register_test(testUsdImagingGLPopOut
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLPopOut --output test test.usda"
    IMAGE_DIFF_COMPARE
        test_0.png
        test_1.png
        test_2.png
        test_3.png
        test_4.png
        test_5.png
        test_6.png
        test_7.png
        test_8.png
    FAIL .05
    FAIL_PERCENT .03
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPopOut
)

pxr_register_test(testUsdImagingGLPopOut_instance
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLPopOut --output test_instance test_instance.usda"
    IMAGE_DIFF_COMPARE
        test_instance_0.png
        test_instance_1.png
        test_instance_2.png
        test_instance_3.png
        test_instance_4.png
        test_instance_5.png
        test_instance_6.png
        test_instance_7.png
        test_instance_8.png
    FAIL .05
    FAIL_PERCENT .03
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPopOut
)

pxr_register_test(testUsdImagingGLPopOut_instance_empty
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLPopOut --output test_instance_empty test_instance_empty.usda"
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPopOut
)

pxr_register_test(testUsdImagingGLMaterialTag
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -lighting -cullStyle back -offscreen -frameAll -stage materialTag.usda -write testUsdImagingGLMaterialTag.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLMaterialTag.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLMaterialTag
)

pxr_register_test(testUsdImagingGLMaterialStrengthOrder
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -camera /camera -lighting -offscreen -stage bindingStrength.usda -write bindingStrength.png"
    IMAGE_DIFF_COMPARE
        bindingStrength.png
    FAIL 0.1
    FAIL_PERCENT 0.002
    WARN 0.01
    WARN_PERCENT 0.0075
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLMaterialStrengthOrder
    # The UsdImagingDelegate did not apply the material override to native instances.
    ENV
        USDIMAGINGGL_ENGINE_ENABLE_SCENE_INDEX=true
)

pxr_register_test(testUsdImagingGLInstancePrimvars
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -frameAll -stage test.usda -write testUsdImagingGLInstancePrimvars.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLInstancePrimvars.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLInstancePrimvars
)

pxr_register_test(testUsdImagingGLInstancePrimvars_Filtering
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -frameAll -lighting -stage testFiltering.usda -write testUsdImagingGLInstancePrimvarsFiltering.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLInstancePrimvarsFiltering.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLInstancePrimvars
)

pxr_register_test(testUsdImagingGLInstancePrimvars_Animated
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -frameAll -times 1 2 3 -stage testAnimated.usda -write testUsdImagingGLInstancePrimvarsAnimated.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLInstancePrimvarsAnimated_001.png
        testUsdImagingGLInstancePrimvarsAnimated_002.png
        testUsdImagingGLInstancePrimvarsAnimated_003.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLInstancePrimvars
)

pxr_register_test(testUsdImagingGLSimpleVolumes
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -rendererAov color -offscreen -stage simpleVolumes.usda -write testUsdImagingGLSimpleVolumes.png -clear 0 0 0 1 -lighting -camera /main_cam -renderSetting volumeRaymarchingStepSize float 0.04 -renderSetting volumeRaymarchingStepSizeLighting float 0.05"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLSimpleVolumes.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLSimpleVolumes
)

pxr_register_test(testUsdImagingGLSimpleVolumes_Glow
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -rendererAov color -offscreen -stage simpleVolumeGlow.usda -write testUsdImagingGLSimpleVolumeGlow.png -clear 0 0 0 1 -lighting -camera /main_cam -renderSetting volumeRaymarchingStepSize float 0.05 -renderSetting volumeRaymarchingStepSizeLighting float 0.05"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLSimpleVolumeGlow.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLSimpleVolumes
)

pxr_register_test(testUsdImagingGLVolumeMaterial
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -rendererAov color -offscreen -stage volumeMaterial.usda -write testUsdImagingGLVolumeMaterial.png -clear 0 0 0 1 -lighting -camera /main_cam -renderSetting volumeRaymarchingStepSize float 0.05 -renderSetting volumeRaymarchingStepSizeLighting float 0.05"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLVolumeMaterial.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLVolumeMaterial
)

pxr_register_test(testUsdImagingGLBasisCurvesVaryingTopology
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -frameAll -offscreen -lighting -times 1 3 4 5 4 -stage bunnyCurves.usda -write testUsdImagingGLBasisCurvesVaryingTopology.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLBasisCurvesVaryingTopology_003.png
        testUsdImagingGLBasisCurvesVaryingTopology_004.png
        testUsdImagingGLBasisCurvesVaryingTopology_005.png
    FAIL .05
    FAIL_PERCENT .03
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLBasisCurvesVaryingTopology
)

# Disable this test temporarily due to issue where test runs forever
pxr_register_test(testUsdImagingGLClippingPlanes
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -frameAll -camera /World/main_cam -times 1 2 -stage clippingPlanes.usda -write testUsdImagingGLClippingPlanes.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLClippingPlanes_001.png
        testUsdImagingGLClippingPlanes_002.png
    FAIL 0.01
    FAIL_PERCENT 0.02
    WARN 0.02
    WARN_PERCENT 0.01
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLClippingPlanes
)

pxr_register_test(testUsdImagingGLAnimatedCamera
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -frameAll -camera /World/main_cam -times 1 2 3 -stage cameraMoving.usda -write testUsdImagingGLAnimatedCamera.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLAnimatedCamera_001.png
        testUsdImagingGLAnimatedCamera_002.png
        testUsdImagingGLAnimatedCamera_003.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLAnimatedCamera
)

pxr_register_test(testUsdImagingGLAnimatedLights
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -frameAll -times 1 2 3 -stage lightMoving.usda -write testUsdImagingGLAnimatedLights.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLAnimatedLights_001.png
        testUsdImagingGLAnimatedLights_002.png
        testUsdImagingGLAnimatedLights_003.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLAnimatedLights
)

pxr_register_test(testUsdImagingGLSdr_1
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -frameAll -stage object.usda -write testUsdImagingGLSdr1.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLSdr1.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLSdr
)

pxr_register_test(testUsdImagingGLSdr_2
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -frameAll -stage usdGlslfxEmbedded.usda -write testUsdImagingGLSdr2.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLSdr2.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLSdr
)

pxr_register_test(testUsdImagingGLInvalidInfoId
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -frameAll -stage invalid_infoid.usda -write testUsdImagingGLInvalidInfoId1.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLInvalidInfoId1.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLInvalidInfoId
)

pxr_register_test(testUsdImagingGLInheritedPurpose_none
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -guidesPurpose hide -renderPurpose hide -proxyPurpose hide -offscreen -lighting -frameAll -stage InheritedPurposes.usd -write testUsdImagingGLInheritedPurpose_none.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLInheritedPurpose_none.png
    FAIL 0.1
    FAIL_PERCENT 0.2
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLInheritedPurpose
)


pxr_register_test(testUsdImagingGLInheritedPurpose_guides
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -guidesPurpose show -renderPurpose hide -proxyPurpose hide -offscreen -lighting -frameAll -stage InheritedPurposes.usd -write testUsdImagingGLInheritedPurpose_guides.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLInheritedPurpose_guides.png
    FAIL 0.1
    FAIL_PERCENT 0.4
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLInheritedPurpose
)


pxr_register_test(testUsdImagingGLInheritedPurpose_render
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -guidesPurpose hide -renderPurpose show -proxyPurpose hide -offscreen -lighting -frameAll -stage InheritedPurposes.usd -write testUsdImagingGLInheritedPurpose_render.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLInheritedPurpose_render.png
    FAIL 0.1
    FAIL_PERCENT 0.4
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLInheritedPurpose
)


pxr_register_test(testUsdImagingGLInheritedPurpose_proxy
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -guidesPurpose hide -renderPurpose hide -proxyPurpose show -offscreen -lighting -frameAll -stage InheritedPurposes.usd -write testUsdImagingGLInheritedPurpose_proxy.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLInheritedPurpose_proxy.png
    FAIL 0.1
    FAIL_PERCENT 0.35
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLInheritedPurpose
)


pxr_register_test(testUsdImagingGLInheritedPurpose_all
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -guidesPurpose show -renderPurpose show -proxyPurpose show -offscreen -lighting -frameAll -stage InheritedPurposes.usd -write testUsdImagingGLInheritedPurpose_all.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLInheritedPurpose_all.png
    FAIL 0.1
    FAIL_PERCENT 0.4
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLInheritedPurpose
)

# NOTE: These 3 tests pass if we allow the perceptual test to pass if we bump up
# the FAIL_PERCENT. From the testSpecs.xml these thresholds are set to 1
# so there might be some differences between idiff -p vs perceptualdiff
pxr_register_test(testUsdImagingGLInheritedPurpose_Instanced_none
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -guidesPurpose hide -renderPurpose hide -proxyPurpose hide -offscreen -lighting -frameAll -stage InheritedPurposes_Instanced.usd -write testUsdImagingGLInheritedPurpose_none.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLInheritedPurpose_none.png
    FAIL 0.1
    FAIL_PERCENT 0.2
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLInheritedPurpose
)


pxr_register_test(testUsdImagingGLInheritedPurpose_Instanced_guides
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -guidesPurpose show -renderPurpose hide -proxyPurpose hide -offscreen -lighting -frameAll -stage InheritedPurposes_Instanced.usd -write testUsdImagingGLInheritedPurpose_guides.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLInheritedPurpose_guides.png
    FAIL 0.1
    FAIL_PERCENT 0.4
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLInheritedPurpose
)


pxr_register_test(testUsdImagingGLInheritedPurpose_Instanced_render
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -guidesPurpose hide -renderPurpose show -proxyPurpose hide -offscreen -lighting -frameAll -stage InheritedPurposes_Instanced.usd -write testUsdImagingGLInheritedPurpose_render.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLInheritedPurpose_render.png
    FAIL 0.1
    FAIL_PERCENT 0.4
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLInheritedPurpose
)


pxr_register_test(testUsdImagingGLInheritedPurpose_Instanced_proxy
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -guidesPurpose hide -renderPurpose hide -proxyPurpose show -offscreen -lighting -frameAll -stage InheritedPurposes_Instanced.usd -write testUsdImagingGLInheritedPurpose_proxy.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLInheritedPurpose_proxy.png
    FAIL 0.1
    FAIL_PERCENT 0.4
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLInheritedPurpose
)


pxr_register_test(testUsdImagingGLInheritedPurpose_Instanced_all
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -guidesPurpose show -renderPurpose show -proxyPurpose show -offscreen -lighting -frameAll -stage InheritedPurposes_Instanced.usd -write testUsdImagingGLInheritedPurpose_all.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLInheritedPurpose_all.png
    FAIL 0.1
    FAIL_PERCENT 0.4
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLInheritedPurpose
)

# Note that in the UsdImagingDelegate, the displayColor authored on a material is stronger than the one authored as primvar on a prim using that material.
pxr_register_test(testUsdImagingGLInvalidUniversalSurface
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -frameAll -stage invalid_universal_surface.usda -write testUsdImagingGLInvalidUniversalSurface.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLInvalidUniversalSurface.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLInvalidUniversalSurface
    ENV
        USDIMAGINGGL_ENGINE_ENABLE_SCENE_INDEX=false
)

# Note that in the stage scene index, the displayColor authored on a material is ignored if there is a displayColor primvar authored on the prim using that material.
pxr_register_test(testUsdImagingGLInvalidUniversalSurface_SceneIndex
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -frameAll -stage invalid_universal_surface.usda -write testUsdImagingGLInvalidUniversalSurface_SceneIndex.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLInvalidUniversalSurface_SceneIndex.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLInvalidUniversalSurface_SceneIndex
    ENV
        USDIMAGINGGL_ENGINE_ENABLE_SCENE_INDEX=true
)

pxr_register_test(testUsdImagingGLNoFallbacks
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -frameAll -stage primitives.usda -write testUsdImagingGLNoFallbacks.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLNoFallbacks.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLFallbackPrimTypes
)


pxr_register_test(testUsdImagingGLFallbacks_1
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -frameAll -stage fallbacks1.usda -write testUsdImagingGLFallbacks_1.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLFallbacks_1.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLFallbackPrimTypes
)


pxr_register_test(testUsdImagingGLFallbacks_2
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -frameAll -stage fallbacks2.usda -write testUsdImagingGLFallbacks_2.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLFallbacks_2.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLFallbackPrimTypes
)


pxr_register_test(testUsdImagingGLFallbacks_3
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -frameAll -stage fallbacks3.usda -write testUsdImagingGLFallbacks_3.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLFallbacks_3.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLFallbackPrimTypes
)

pxr_register_test(testUsdImagingGLPrimvarProcessing
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -frameAll -times 0 1 2 3   -stage points_widths.usda -write testUsdImagingGLPrimvarProcessing_points_widths.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLPrimvarProcessing_points_widths_000.png
        testUsdImagingGLPrimvarProcessing_points_widths_001.png
        testUsdImagingGLPrimvarProcessing_points_widths_002.png
        testUsdImagingGLPrimvarProcessing_points_widths_003.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPrimvarProcessing
)

pxr_register_test(testUsdImagingGLComposite
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -frameAll -times 0 1 2 -stage moving_cubes.usda -camera /main_cam -presentComposite -rendererAov color -write testUsdImagingGLComposite.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLComposite_000.png
        testUsdImagingGLComposite_001.png
        testUsdImagingGLComposite_002.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLComposite
)

pxr_register_test(testUsdImagingGLPresentDisabled
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -presentDisabled -offscreen -lighting -frameAll -stage sphere.usda -rendererAov color -write testUsdImagingGLPresentDisabled.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLPresentDisabled.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPresentDisabled
)

pxr_register_test(testUsdImagingGLFraming_1
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -rendererAov color -offscreen -stage framing.usda -clear 0 0 0 1 -lighting -camera /main_cam -dataWindow 0 0 640 480 -displayWindow 0 0 640 480 -windowPolicy fit -write testUsdImagingGLFraming1.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLFraming1.png
    FAIL 0.01
    FAIL_PERCENT 0.04
    WARN 0.02
    WARN_PERCENT 0.02
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLFraming
)


pxr_register_test(testUsdImagingGLFraming_2
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -rendererAov color -offscreen -stage framing.usda -clear 0 0 0 1 -lighting -camera /main_cam -dataWindow 0 40 640 430 -displayWindow 0 0 1280 960 -windowPolicy fit -write testUsdImagingGLFraming2.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLFraming2.png
    FAIL 0.01
    FAIL_PERCENT 0.04
    WARN 0.02
    WARN_PERCENT 0.02
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLFraming
)


pxr_register_test(testUsdImagingGLFraming_3
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -rendererAov color -offscreen -stage framing.usda -clear 0 0 0 1 -lighting -camera /main_cam -dataWindow 0 0 640 480 -displayWindow 0 0 320 480 -windowPolicy fit -write testUsdImagingGLFraming3.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLFraming3.png
    FAIL 0.01
    FAIL_PERCENT 0.04
    WARN 0.02
    WARN_PERCENT 0.02
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLFraming
)


pxr_register_test(testUsdImagingGLFraming_4
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -rendererAov color -offscreen -stage framing.usda -clear 0 0 0 1 -lighting -camera /main_cam -dataWindow 0 0 640 480 -displayWindow 0 0 320 480 -windowPolicy fit -pixelAspectRatio 1.5 -write testUsdImagingGLFraming4.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLFraming4.png
    FAIL 0.01
    FAIL_PERCENT 0.04
    WARN 0.02
    WARN_PERCENT 0.02
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLFraming
)


pxr_register_test(testUsdImagingGLFraming_5
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -rendererAov color -offscreen -stage framing.usda -clear 0 0 0 1 -lighting -camera /main_cam -dataWindow 25 136 367 298 -displayWindow 50 80 320 400 -windowPolicy fit -write testUsdImagingGLFraming5.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLFraming5.png
    FAIL 0.01
    FAIL_PERCENT 0.04
    WARN 0.02
    WARN_PERCENT 0.02
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLFraming
)


pxr_register_test(testUsdImagingGLFraming_6
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -rendererAov color -offscreen -stage framing.usda -clear 0 0 0 1 -lighting -camera /OrthoCam -dataWindow 25 136 367 298 -displayWindow 50 80 320 400 -windowPolicy fit -write testUsdImagingGLFraming6.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLFraming6.png
    FAIL 0.01
    FAIL_PERCENT 0.04
    WARN 0.02
    WARN_PERCENT 0.02
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLFraming
)

pxr_register_test(testUsdImagingGLHWFaceCulling
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -times 0 1 2 3 -cullStyle backUnlessDoubleSided -shading smooth -frameAll -stage testHWFaceCulling.usda -write testHWFaceCulling.png"
    IMAGE_DIFF_COMPARE
        testHWFaceCulling_000.png
        testHWFaceCulling_001.png
        testHWFaceCulling_002.png
        testHWFaceCulling_003.png
    FAIL 0.05
    FAIL_PERCENT 0.001
    WARN 0.05
    WARN_PERCENT 0.0005
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLHWFaceCulling
)

# The following test is disabled since the stages are unable to load any .tx files
# In theory these image files should be able to load through the hioOiio plugin but I have not
# been able to get that too work. I did try to convert these images through OIIO's iconvert tool
# but it also fails trying to open the .tx files
#pxr_register_test(testUsdImagingGLMipmap
#    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -stage testUsdImagingGLMipmap.usda -write testUsdImagingGLMipmap.png -times 3 6 10 14 17 -clear 0 0 0 1 -lighting -camera /main_cam"
#    IMAGE_DIFF_COMPARE
#        testUsdImagingGLMipmap_003.png
#        testUsdImagingGLMipmap_006.png
#        testUsdImagingGLMipmap_010.png
#        testUsdImagingGLMipmap_014.png
#        testUsdImagingGLMipmap_017.png
#    FAIL 0.05
#    FAIL_PERCENT 0.001
#    WARN 0.025
#    WARN_PERCENT 0.002
#    EXPECTED_RETURN_CODE 0
#    TESTENV testUsdImagingGLMipmap
#)

pxr_register_test(testUsdImagingGLPrimvarRefinement_Uniform_Low
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -complexity 1.1 -offscreen -lighting -camlight -shading wireOnSurface -frameAll -stage testUsdImagingGLPrimvarRefinement.usda -write testUsdImagingGLPrimvarRefinement_Uniform_Low.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLPrimvarRefinement_Uniform_Low.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPrimvarRefinement
)


pxr_register_test(testUsdImagingGLPrimvarRefinement_Uniform_Medium
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -complexity 1.2 -offscreen -lighting -camlight -shading wireOnSurface -frameAll -stage testUsdImagingGLPrimvarRefinement.usda -write testUsdImagingGLPrimvarRefinement_Uniform_Medium.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLPrimvarRefinement_Uniform_Medium.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPrimvarRefinement
)

pxr_register_test(testUsdImagingGLDisplacement_Uniform_VeryHigh
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -complexity 1.4 -offscreen -lighting -sceneLights -shading wireOnSurface -camera /Camera -stage testUsdImagingGLDisplacement.usda -write testUsdImagingGLDisplacement_Uniform_VeryHigh.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLDisplacement_Uniform_VeryHigh.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLDisplacement
)

pxr_register_test(testUsdImagingGLPrimvarRefinement_Adaptive_Low
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -complexity 1.1 -offscreen -lighting -camlight -shading wireOnSurface -frameAll -stage testUsdImagingGLPrimvarRefinement.usda -write testUsdImagingGLPrimvarRefinement_Adaptive_Low.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLPrimvarRefinement_Adaptive_Low.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPrimvarRefinement
    ENV
        HD_ENABLE_OPENSUBDIV3_ADAPTIVE=1
)


pxr_register_test(testUsdImagingGLPrimvarRefinement_Adaptive_Medium
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -complexity 1.2 -offscreen -lighting -camlight -shading wireOnSurface -frameAll -stage testUsdImagingGLPrimvarRefinement.usda -write testUsdImagingGLPrimvarRefinement_Adaptive_Medium.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLPrimvarRefinement_Adaptive_Medium.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLPrimvarRefinement
    ENV
        HD_ENABLE_OPENSUBDIV3_ADAPTIVE=1
)

pxr_register_test(testUsdImagingGLDisplacement_Adaptive_VeryHigh
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -complexity 1.4 -offscreen -lighting -sceneLights -shading wireOnSurface -camera /Camera -stage testUsdImagingGLDisplacement.usda -write testUsdImagingGLDisplacement_Adaptive_VeryHigh.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLDisplacement_Adaptive_VeryHigh.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLDisplacement
    ENV
        HD_ENABLE_OPENSUBDIV3_ADAPTIVE=1
)

pxr_register_test(testUsdImagingGLGeomSubsets_Low
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -complexity 1.0 -offscreen -lighting -camlight -shading wireOnSurface -frameAll -stage testUsdImagingGLGeomSubsets.usda -write testUsdImagingGLGeomSubsets_Low.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLGeomSubsets_Low.png
    FAIL 1
    FAIL_PERCENT 2
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLGeomSubsets
)


pxr_register_test(testUsdImagingGLGeomSubsets_Medium
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -complexity 1.1 -offscreen -lighting -camlight -shading wireOnSurface -frameAll -stage testUsdImagingGLGeomSubsets.usda -write testUsdImagingGLGeomSubsets_Medium.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLGeomSubsets_Medium.png
    FAIL 1
    FAIL_PERCENT 2
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLGeomSubsets
)

pxr_register_test(testUsdImagingGLGeomSubsets_Points
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -camlight -shading points -frameAll -stage testUsdImagingGLGeomSubsets.usda -write testUsdImagingGLGeomSubsets_Points.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLGeomSubsets_Points.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLGeomSubsets
)

pxr_register_test(testUsdImagingGLGeomSubsets_Quadrangulated
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -camlight -shading wireOnSurface -frameAll -stage testUsdImagingGLGeomSubsets.usda -write testUsdImagingGLGeomSubsets_Quadrangulated.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLGeomSubsets_Quadrangulated.png
    FAIL 1
    FAIL_PERCENT 2
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLGeomSubsets
    ENV
        HD_ENABLE_FORCE_QUADRANGULATE=1
)


pxr_register_test(testUsdImagingGLGeomSubsets_QuadrangulatedPoints
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -camlight -shading points -frameAll -stage testUsdImagingGLGeomSubsets.usda -write testUsdImagingGLGeomSubsets_QuadrangulatedPoints.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLGeomSubsets_QuadrangulatedPoints.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLGeomSubsets
    ENV
        HD_ENABLE_FORCE_QUADRANGULATE=1
)

pxr_register_test(testUsdImagingGLGeomSubsets_Low_SceneIndex
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -complexity 1.0 -offscreen -lighting -camlight -shading wireOnSurface -frameAll -stage testUsdImagingGLGeomSubsets.usda -write testUsdImagingGLGeomSubsets_Low.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLGeomSubsets_Low.png
    FAIL 1
    FAIL_PERCENT 2
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLGeomSubsets_SceneIndex
    ENV
        USDIMAGINGGL_ENGINE_ENABLE_SCENE_INDEX=1
)


pxr_register_test(testUsdImagingGLGeomSubsets_Medium_SceneIndex
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -complexity 1.1 -offscreen -lighting -camlight -shading wireOnSurface -frameAll -stage testUsdImagingGLGeomSubsets.usda -write testUsdImagingGLGeomSubsets_Medium.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLGeomSubsets_Medium.png
    FAIL 1
    FAIL_PERCENT 2
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLGeomSubsets_SceneIndex
    ENV
        USDIMAGINGGL_ENGINE_ENABLE_SCENE_INDEX=1
)

pxr_register_test(testUsdImagingGLGeomSubsets_Points_SceneIndex
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -camlight -shading points -frameAll -stage testUsdImagingGLGeomSubsets.usda -write testUsdImagingGLGeomSubsets_Points.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLGeomSubsets_Points.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLGeomSubsets_SceneIndex
    ENV
        USDIMAGINGGL_ENGINE_ENABLE_SCENE_INDEX=1
)

pxr_register_test(testUsdImagingGLGeomSubsets_Quadrangulated_SceneIndex
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -camlight -shading wireOnSurface -frameAll -stage testUsdImagingGLGeomSubsets.usda -write testUsdImagingGLGeomSubsets_Quadrangulated.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLGeomSubsets_Quadrangulated.png
    FAIL 1
    FAIL_PERCENT 2
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLGeomSubsets_SceneIndex
    ENV
        HD_ENABLE_FORCE_QUADRANGULATE=1
        USDIMAGINGGL_ENGINE_ENABLE_SCENE_INDEX=1
)


pxr_register_test(testUsdImagingGLGeomSubsets_QuadrangulatedPoints_SceneIndex
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -camlight -shading points -frameAll -stage testUsdImagingGLGeomSubsets.usda -write testUsdImagingGLGeomSubsets_QuadrangulatedPoints.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLGeomSubsets_QuadrangulatedPoints.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLGeomSubsets_SceneIndex
    ENV
        HD_ENABLE_FORCE_QUADRANGULATE=1
        USDIMAGINGGL_ENGINE_ENABLE_SCENE_INDEX=1
)

pxr_register_test(testUsdImagingGLAovVisualization_Storm_depth
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -stage viz_aov.usda -rendererAov depth -clear 0 0 0 1 -lighting -camera /main_cam -write testUsdImagingGLAovVisualization_Storm_depth.png"
    IMAGE_DIFF_COMPARE
        testUsdImagingGLAovVisualization_Storm_depth.png
    FAIL 0.05
    FAIL_PERCENT 0.001
    WARN 0.025
    WARN_PERCENT 0.002
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLAovVisualization
)

pxr_register_test(testUsdImagingGLInvalidMaterial
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -shading smooth  -frameAll -stage invalidMaterial.usda -write testUsdImagingGLInvalidMaterial.png"
    IMAGE_DIFF_COMPARE
    testUsdImagingGLInvalidMaterial.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    TESTENV testUsdImagingGLInvalidMaterial
)

pxr_register_test(testUsdImagingGLDomeLightTaskController
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLTaskController --lightTexture testImage.png --stage test.usda"
    IMAGE_DIFF_COMPARE
        initialDome.png
        rotatedDome.png
    FAIL 0.01
    FAIL_PERCENT 0.2
    WARN 0.02
    WARN_PERCENT 0.005
    TESTENV testUsdImagingGLDomeLightTaskController
)

#
# Conditionally install and register MaterialX tests
#
if (MaterialX_FOUND AND ${PXR_ENABLE_MATERIALX_SUPPORT})
    pxr_install_test_dir(
        SRC testenv/testUsdImagingGLMaterialXBasic
        DEST testUsdImagingGLMaterialXBasic
    )

    pxr_register_test(testUsdImagingGLMaterialXBasic_mxSimple
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -lighting -sceneLights -shading smooth -complexity 1.3 -offscreen -frameAll -stage mxSimple.usda -write mxSimple.png"
        IMAGE_DIFF_COMPARE
            mxSimple.png
        FAIL 0.3
        FAIL_PERCENT 0.005
        WARN 0.05
        WARN_PERCENT 0.03
        EXPECTED_RETURN_CODE 0
        TESTENV testUsdImagingGLMaterialXBasic
    )

    pxr_register_test(testUsdImagingGLMaterialXBasic_standardSurface
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -lighting -sceneLights -shading smooth -complexity 1.3 -offscreen -frameAll -stage standardSurface.usda -write standardSurface.png"
        IMAGE_DIFF_COMPARE
            standardSurface.png
        FAIL 0.3
        FAIL_PERCENT 0.005
        WARN 0.05
        WARN_PERCENT 0.2
        EXPECTED_RETURN_CODE 0
        TESTENV testUsdImagingGLMaterialXBasic
    )

    pxr_register_test(testUsdImagingGLMaterialXBasic_standardSurfaceMarble
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -lighting -sceneLights -shading smooth -complexity 1.3 -offscreen -frameAll -stage standardSurfaceMarble.usda -write standardSurfaceMarble.png"
        IMAGE_DIFF_COMPARE
            standardSurfaceMarble.png
        FAIL 0.1
        FAIL_PERCENT 0.005
        WARN 0.05
        WARN_PERCENT 0.01
        EXPECTED_RETURN_CODE 0
        TESTENV testUsdImagingGLMaterialXBasic
    )

    pxr_register_test(testUsdImagingGLMaterialXBasic_usdPreviewSurface
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -lighting -sceneLights -shading smooth -complexity 1.3 -offscreen -frameAll -stage usdPreviewSurface.usda -write usdPreviewSurface.png"
        IMAGE_DIFF_COMPARE
            usdPreviewSurface.png
        FAIL 0.3
        FAIL_PERCENT 0.005
        WARN 0.05
        WARN_PERCENT 0.09
        EXPECTED_RETURN_CODE 0
        TESTENV testUsdImagingGLMaterialXBasic
    )

    pxr_register_test(testUsdImagingGLMaterialXBasic_mxGlass
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -lighting -sceneLights -shading smooth -complexity 1.3 -offscreen -frameAll -stage mxGlass.usda -write mxGlass.png"
        IMAGE_DIFF_COMPARE
            mxGlass.png
        FAIL 0.3
        FAIL_PERCENT 0.005
        WARN 0.05
        WARN_PERCENT 0.02
        EXPECTED_RETURN_CODE 0
        TESTENV testUsdImagingGLMaterialXBasic
    )

    pxr_register_test(testUsdImagingGLMaterialXBasic_basicMxYup
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -lighting -sceneLights -shading smooth -complexity 1.3 -offscreen -stage basicMxYup.usda -camera /FrontCamera -write basicMxYup.png"
        IMAGE_DIFF_COMPARE
            basicMxYup.png
        FAIL 0.1
        FAIL_PERCENT 0.005
        WARN 0.05
        WARN_PERCENT 0.0025
        EXPECTED_RETURN_CODE 0
        TESTENV testUsdImagingGLMaterialXBasic
    )

    pxr_register_test(testUsdImagingGLMaterialXBasic_basicMxZup
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -lighting -sceneLights -shading smooth -complexity 1.3 -offscreen -stage basicMxZup.usda -camera /FrontCamera -write basicMxZup.png"
        IMAGE_DIFF_COMPARE
            basicMxZup.png
        FAIL 0.1
        FAIL_PERCENT 0.005
        WARN 0.05
        WARN_PERCENT 0.0025
        EXPECTED_RETURN_CODE 0
        TESTENV testUsdImagingGLMaterialXBasic
    )

    pxr_register_test(testUsdImagingGLMaterialXBasic_mxDirectLights
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -lighting -sceneLights -shading smooth -complexity 1.3 -offscreen -frameAll -stage mxDirectLights.usda -write mxDirectLights.png"
        IMAGE_DIFF_COMPARE
            mxDirectLights.png
        FAIL 0.1
        FAIL_PERCENT 0.005
        WARN 0.05
        WARN_PERCENT 0.0025
        EXPECTED_RETURN_CODE 0
        TESTENV testUsdImagingGLMaterialXBasic
    )

    pxr_register_test(testUsdImagingGLMaterialXBasic_mxDirectLightsCameraLight
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -lighting -camlight -shading smooth -complexity 1.3 -offscreen -frameAll -stage mxDirectLights.usda -write mxDirectLightsCameraLight.png"
        IMAGE_DIFF_COMPARE
            mxDirectLightsCameraLight.png
        FAIL 0.1
        FAIL_PERCENT 0.005
        WARN 0.05
        WARN_PERCENT 0.0025
        EXPECTED_RETURN_CODE 0
        TESTENV testUsdImagingGLMaterialXBasic
    )

    pxr_register_test(testUsdImagingGLMaterialXBasic_mxLights
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -lighting -sceneLights -shading smooth -complexity 1.3 -offscreen -frameAll -stage mxLights.usda -write mxLights.png"
        IMAGE_DIFF_COMPARE
            mxLights.png
        FAIL 0.1
        FAIL_PERCENT 0.005
        WARN 0.05
        WARN_PERCENT 0.02
        EXPECTED_RETURN_CODE 0
        TESTENV testUsdImagingGLMaterialXBasic
    )

    pxr_register_test(testUsdImagingGLMaterialXBasic_mxColorspaces
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -lighting -sceneLights -shading smooth -complexity 1.3 -offscreen -frameAll -stage mxColorspaces.usda -write mxColorspaces.png"
        IMAGE_DIFF_COMPARE
            mxColorspaces.png
        FAIL 0.1
        FAIL_PERCENT 0.005
        WARN 0.05
        WARN_PERCENT 0.02
        EXPECTED_RETURN_CODE 0
        TESTENV testUsdImagingGLMaterialXBasic
    )

    pxr_register_test(testUsdImagingGLMaterialXBasic_normalMap
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -lighting -sceneLights -shading smooth -complexity 1.3 -offscreen -frameAll -stage mxNormalMap.usda -write mxNormalMap.png"
        IMAGE_DIFF_COMPARE
            mxNormalMap.png
        FAIL 0.1
        FAIL_PERCENT 0.005
        WARN 0.05
        WARN_PERCENT 0.03
        EXPECTED_RETURN_CODE 0
        TESTENV testUsdImagingGLMaterialXBasic
    )
    
    # MaterialX materials using texture, texture coordinates, and geompropvalues.
    pxr_install_test_dir(
        SRC testenv/testUsdImagingGLMaterialXTextured
        DEST testUsdImagingGLMaterialXTextured
    )


    pxr_register_test(testUsdImagingGLMaterialXTextured_mxTextured
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -lighting -sceneLights -shading smooth -complexity 1.3 -offscreen -frameAll -stage mxTextured.usda -write mxTextured.png"
        IMAGE_DIFF_COMPARE
            mxTextured.png
        FAIL 0.1
        FAIL_PERCENT 0.005
        WARN 0.05
        WARN_PERCENT 0.05
        EXPECTED_RETURN_CODE 0
        TESTENV testUsdImagingGLMaterialXTextured
    )

    pxr_register_test(testUsdImagingGLMaterialXTextured_mxTextured2
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -lighting -sceneLights -shading smooth -complexity 1.3 -offscreen -frameAll -stage mxTextured2.usda -write mxTextured2.png"
        IMAGE_DIFF_COMPARE
            mxTextured2.png
        FAIL 0.1
        FAIL_PERCENT 0.005
        WARN 0.05
        WARN_PERCENT 0.0025
        EXPECTED_RETURN_CODE 0
        TESTENV testUsdImagingGLMaterialXTextured
    )

    pxr_register_test(testUsdImagingGLMaterialXTextured_mxGeomprop
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -lighting -sceneLights -shading smooth -complexity 1.3 -offscreen -frameAll -stage mxGeomprop.usda -write mxGeomprop.png"
        IMAGE_DIFF_COMPARE
            mxGeomprop.png
        FAIL 0.1
        FAIL_PERCENT 0.005
        WARN 0.05
        WARN_PERCENT 0.02
        EXPECTED_RETURN_CODE 0
        TESTENV testUsdImagingGLMaterialXTextured
    )

    pxr_register_test(testUsdImagingGLMaterialXTextured_mxTexcoord
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -lighting -sceneLights -shading smooth -complexity 1.3 -offscreen -frameAll -stage mxTexcoord.usda -write mxTexcoord.png"
        IMAGE_DIFF_COMPARE
            mxTexcoord.png
        FAIL 0.1
        FAIL_PERCENT 0.005
        WARN 0.05
        WARN_PERCENT 0.02
        EXPECTED_RETURN_CODE 0
        TESTENV testUsdImagingGLMaterialXTextured
    )

    pxr_register_test(testUsdImagingGLMaterialXTextured_mxDefaultTextureVals
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -lighting -sceneLights -shading smooth -complexity 1.3 -offscreen -frameAll -stage mxDefaultTextureVals.usda -write mxDefaultTextureVals.png"
        IMAGE_DIFF_COMPARE
            mxDefaultTextureVals.png
        FAIL 0.5
        FAIL_PERCENT 0.005
        WARN 0.05
        WARN_PERCENT 0.02
        EXPECTED_RETURN_CODE 0
        TESTENV testUsdImagingGLMaterialXTextured
    )

    pxr_register_test(testUsdImagingGLMaterialXTextured_mxFallbackDome
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -lighting -sceneLights -shading smooth -complexity 1.3 -offscreen -frameAll -stage mxFallbackDome.usda -write mxFallbackDome.png"
        IMAGE_DIFF_COMPARE
            mxFallbackDome.png
        FAIL 0.5
        FAIL_PERCENT 0.005
        WARN 0.05
        WARN_PERCENT 0.02
        EXPECTED_RETURN_CODE 0
        TESTENV testUsdImagingGLMaterialXTextured
    )

    pxr_register_test(testUsdImagingGLMaterialXTextured_mxDefaultGemopropVal
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -lighting -sceneLights -shading smooth -complexity 1.3 -offscreen -camera /Camera -stage mxDefaultGeompropVal.usda -write mxDefaultGeompropVal.png"
        IMAGE_DIFF_COMPARE
            mxDefaultGeompropVal.png
        FAIL 0.1
        FAIL_PERCENT 0.005
        WARN 0.05
        WARN_PERCENT 0.02
        EXPECTED_RETURN_CODE 0
        TESTENV testUsdImagingGLMaterialXTextured
    )

    pxr_register_test(testUsdImagingGLMaterialXTextured_mxCustomNodeDef
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -lighting -sceneLights -shading smooth -complexity 1.3 -offscreen -camera /Camera -stage mxCustomNodeDef.usda -write mxCustomNodeDef.png"
        IMAGE_DIFF_COMPARE
            mxCustomNodeDef.png
        FAIL 0.5
        FAIL_PERCENT 0.005
        WARN 0.05
        WARN_PERCENT 0.02
        EXPECTED_RETURN_CODE 0
        TESTENV testUsdImagingGLMaterialXTextured
    )

    pxr_register_test(testUsdImagingGLMaterialXTextured_mxTexColorspaces
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -lighting -sceneLights -shading smooth -complexity 1.3 -offscreen -frameAll -stage mxTexColorspaces.usda -write mxTexColorspaces.png"
        IMAGE_DIFF_COMPARE
            mxTexColorspaces.png
        FAIL 0.1
        FAIL_PERCENT 0.005
        WARN 0.05
        WARN_PERCENT 0.02
        EXPECTED_RETURN_CODE 0
        TESTENV testUsdImagingGLMaterialXTextured
    )
    
# These two tests are to provide direct comparisons between the Native and  
# MaterialX implementations of PreviewSurface Materials. 
# The first test is using purely Native PreviewSurface materials. 
# The second uses identical scenes with equivalent MaterialX PreviewSurface
# materials. 
# Native UsdPreviewSurface Materials:
    # pxr_install_test_dir(
    #     SRC testenv/testUsdImagingGLMaterialXvsNative
    #     DEST testUsdImagingGLMaterialXvsNative_native
    # )

    # pxr_register_test(testUsdImagingGLMaterialXvsNative_nativeDefault
    #     COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -lighting -sceneLights -shading smooth -complexity 1.3 -offscreen -camera /Camera -stage nativeDefault.usda -write default.png"
    #     IMAGE_DIFF_COMPARE
    #         default.png
    #     FAIL 0.5
    #     FAIL_PERCENT 0.005
    #     WARN 0.05
    #     WARN_PERCENT 0.0075
    #     EXPECTED_RETURN_CODE 0
    #     TESTENV testUsdImagingGLMaterialXvsNative
    # )

    # pxr_register_test(testUsdImagingGLMaterialXvsNative_nativeMmetallic
    #     COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -lighting -sceneLights -shading smooth -complexity 1.3 -offscreen -camera /Camera -stage nativeMmetallic.usda -write metallic.png"
    #     IMAGE_DIFF_COMPARE
    #         metallic.png
    #     FAIL 0.5
    #     FAIL_PERCENT 0.005
    #     WARN 0.05
    #     WARN_PERCENT 0.6
    #     EXPECTED_RETURN_CODE 0
    #     TESTENV testUsdImagingGLMaterialXvsNative
    # )

    # pxr_register_test(testUsdImagingGLMaterialXvsNative_nativeWorkflow
    #     COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -lighting -sceneLights -shading smooth -complexity 1.3 -offscreen -camera /Camera -stage nativeWorkflow.usda -write workflow.png"
    #     IMAGE_DIFF_COMPARE
    #         workflow.png
    #     FAIL 0.5
    #     FAIL_PERCENT 0.005
    #     WARN 0.05
    #     WARN_PERCENT 0.2
    #     EXPECTED_RETURN_CODE 0
    #     TESTENV testUsdImagingGLMaterialXvsNative
    # )

    # pxr_register_test(testUsdImagingGLMaterialXvsNative_nativeOpacity
    #     COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -lighting -sceneLights -shading smooth -complexity 1.3 -offscreen -camera /Camera -stage nativeOpacity.usda -write opacity.png"
    #     IMAGE_DIFF_COMPARE
    #         opacity.png
    #     FAIL 0.5
    #     FAIL_PERCENT 0.005
    #     WARN 0.05
    #     WARN_PERCENT 0.2
    #     EXPECTED_RETURN_CODE 0
    #     TESTENV testUsdImagingGLMaterialXvsNative
    # )

    # pxr_register_test(testUsdImagingGLMaterialXvsNative_nativeClearcoat
    #     COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -lighting -sceneLights -shading smooth -complexity 1.3 -offscreen -camera /Camera -stage clearcoatNative.usda -write clearcoat.png"
    #     IMAGE_DIFF_COMPARE
    #         clearcoat.png
    #     FAIL 0.5
    #     FAIL_PERCENT 0.005
    #     WARN 0.05
    #     WARN_PERCENT 0.6
    #     EXPECTED_RETURN_CODE 0
    #     TESTENV testUsdImagingGLMaterialXvsNative
    # )

    # pxr_register_test(testUsdImagingGLMaterialXvsNative_nativeEmissive
    #     COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -lighting -sceneLights -shading smooth -complexity 1.3 -offscreen -camera /Camera -stage nativeEmissive.usda -write emissive.png"
    #     IMAGE_DIFF_COMPARE
    #         emissive.png
    #     FAIL 0.5
    #     FAIL_PERCENT 0.005
    #     WARN 0.05
    #     WARN_PERCENT 0.2
    #     EXPECTED_RETURN_CODE 0
    #     TESTENV testUsdImagingGLMaterialXvsNative
    # )

    # pxr_register_test(testUsdImagingGLMaterialXvsNative_nativeNormalMap
    #     COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -lighting -sceneLights -shading smooth -complexity 1.3 -offscreen -camera /Camera -stage nativeNormalMap.usda -write normalMapped.png"
    #     IMAGE_DIFF_COMPARE
    #         normalMapped.png
    #     FAIL 0.5
    #     FAIL_PERCENT 0.005
    #     WARN 0.05
    #     WARN_PERCENT 0.2
    #     EXPECTED_RETURN_CODE 0
    #     TESTENV testUsdImagingGLMaterialXvsNative
    # )

# MaterialX UsdPreviewSurface Materials: 
    # pxr_install_test_dir(
    #     SRC testenv/testUsdImagingGLMaterialXvsNative
    #     DEST testUsdImagingGLMaterialXvsNative_materialX
    # )
    
    # pxr_register_test(testUsdImagingGLMaterialXvsNative_materialXdefault
    #     COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -lighting -sceneLights -shading smooth -complexity 1.3 -offscreen -camera /Camera -stage materialXdefault.usda -write materialXdefault.png"
    #     IMAGE_DIFF_COMPARE
    #         default.png
    #     FAIL 0.5
    #     FAIL_PERCENT 0.005
    #     WARN 0.05
    #     WARN_PERCENT 0.0075
    #     EXPECTED_RETURN_CODE 0
    #     TESTENV testUsdImagingGLMaterialXvsNative
    # )

    # pxr_register_test(testUsdImagingGLMaterialXvsNative_materialXmetallic
    #     COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -lighting -sceneLights -shading smooth -complexity 1.3 -offscreen -camera /Camera -stage materialXmetallic.usda -write materialXmetallic.png"
    #     IMAGE_DIFF_COMPARE
    #         metallic.png
    #     FAIL 0.5
    #     FAIL_PERCENT 0.005
    #     WARN 0.05
    #     WARN_PERCENT 0.6
    #     EXPECTED_RETURN_CODE 0
    #     TESTENV testUsdImagingGLMaterialXvsNative
    # )

    # pxr_register_test(testUsdImagingGLMaterialXvsNative_materialXworkflow
    #     COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -lighting -sceneLights -shading smooth -complexity 1.3 -offscreen -camera /Camera -stage materialXworkflow.usda -write materialXworkflow.png"
    #     IMAGE_DIFF_COMPARE
    #         workflow.png
    #     FAIL 0.5
    #     FAIL_PERCENT 0.005
    #     WARN 0.05
    #     WARN_PERCENT 0.2
    #     EXPECTED_RETURN_CODE 0
    #     TESTENV testUsdImagingGLMaterialXvsNative
    # )

    # pxr_register_test(testUsdImagingGLMaterialXvsNative_materialXopacity
    #     COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -lighting -sceneLights -shading smooth -complexity 1.3 -offscreen -camera /Camera -stage materialXopacity.usda -write materialXopacity.png"
    #     IMAGE_DIFF_COMPARE
    #         opacity.png
    #     FAIL 0.5
    #     FAIL_PERCENT 0.005
    #     WARN 0.05
    #     WARN_PERCENT 0.2
    #     EXPECTED_RETURN_CODE 0
    #     TESTENV testUsdImagingGLMaterialXvsNative
    # )

    # pxr_register_test(testUsdImagingGLMaterialXvsNative_materialXclearcoat
    #     COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -lighting -sceneLights -shading smooth -complexity 1.3 -offscreen -camera /Camera -stage materialXclearcoat.usda -write materialXclearcoat.png"
    #     IMAGE_DIFF_COMPARE
    #         clearcoat.png
    #     FAIL 0.5
    #     FAIL_PERCENT 0.005
    #     WARN 0.05
    #     WARN_PERCENT 0.6
    #     EXPECTED_RETURN_CODE 0
    #     TESTENV testUsdImagingGLMaterialXvsNative
    # )

    # pxr_register_test(testUsdImagingGLMaterialXvsNative_materialXemissive
    #     COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -lighting -sceneLights -shading smooth -complexity 1.3 -offscreen -camera /Camera -stage materialXemissive.usda -write materialXemissive.png"
    #     IMAGE_DIFF_COMPARE
    #         emissive.png
    #     FAIL 0.5
    #     FAIL_PERCENT 0.005
    #     WARN 0.05
    #     WARN_PERCENT 0.2
    #     EXPECTED_RETURN_CODE 0
    #     TESTENV testUsdImagingGLMaterialXvsNative
    # )

    # pxr_register_test(testUsdImagingGLMaterialXvsNative_materialXnormalMap
    #     COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -lighting -sceneLights -shading smooth -complexity 1.3 -offscreen -camera /Camera -stage materialXnormalMap.usda -write materialXnormalMap.png"
    #     IMAGE_DIFF_COMPARE
    #         normalMapped.png
    #     FAIL 0.5
    #     FAIL_PERCENT 0.005
    #     WARN 0.05
    #     WARN_PERCENT 0.2
    #     EXPECTED_RETURN_CODE 0
    #     TESTENV testUsdImagingGLMaterialXvsNative
    # )

    pxr_install_test_dir(
        SRC testenv/testUsdImagingGLMaterialXCustomNodes
        DEST testUsdImagingGLMaterialXCustomNodes
    )
        
    pxr_register_test(testUsdImagingGLMaterialXCustomNodes_customTexNode
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -lighting -sceneLights -shading smooth -complexity 1.3 -offscreen -camera /Camera -stage customTexNode.usda -write customTexNode.png"
        IMAGE_DIFF_COMPARE
            customTexNode.png
        FAIL 0.5
        FAIL_PERCENT 0.005
        WARN 0.05
        WARN_PERCENT 0.2
        EXPECTED_RETURN_CODE 0
        TESTENV testUsdImagingGLMaterialXCustomNodes
        ENV
            PXR_MTLX_PLUGIN_SEARCH_PATHS=./customNodeDefs
    )

    pxr_register_test(testUsdImagingGLMaterialXCustomNodes_customMaterial
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -lighting -sceneLights -shading smooth -complexity 1.3 -offscreen -camera /Camera -stage customMaterial.usda -write customMaterial.png"
        IMAGE_DIFF_COMPARE
            customMaterial.png
        FAIL 0.1
        FAIL_PERCENT 0.005
        WARN 0.05
        WARN_PERCENT 0.2
        EXPECTED_RETURN_CODE 0
        TESTENV testUsdImagingGLMaterialXCustomNodes
        ENV
            PXR_MTLX_PLUGIN_SEARCH_PATHS=./customNodeDefs
    )

    pxr_register_test(testUsdImagingGLMaterialXCustomNodes_customColor
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -lighting -sceneLights -shading smooth -complexity 1.3 -offscreen -camera /Camera -stage customColor.usda -write customColor.png"
        IMAGE_DIFF_COMPARE
            customColor.png
        FAIL 0.1
        FAIL_PERCENT 0.005
        WARN 0.05
        WARN_PERCENT 0.2
        EXPECTED_RETURN_CODE 0
        TESTENV testUsdImagingGLMaterialXCustomNodes
        ENV
            PXR_MTLX_PLUGIN_SEARCH_PATHS=./customNodeDefs
    )
    
    # Not a MaterialX test as such, but requires MaterialX support
    pxr_register_test(testUsdImagingGLGeomSubsets_Primvars_SceneIndex
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -shading wireOnSurface -stage primvars.usda -write testUsdImagingGLGeomSubsets_Primvars.png"
        IMAGE_DIFF_COMPARE
            testUsdImagingGLGeomSubsets_Primvars.png
        FAIL 1
        FAIL_PERCENT 1
        PERCEPTUAL
        EXPECTED_RETURN_CODE 0
        TESTENV testUsdImagingGLGeomSubsets_SceneIndex
        ENV
            USDIMAGINGGL_ENGINE_ENABLE_SCENE_INDEX=1
    )

endif() # MaterialX tests

#
# Conditionally install and register Embree tests
#
if (EMBREE_FOUND AND ${PXR_BUILD_EMBREE_PLUGIN})
    pxr_install_test_dir(
        SRC testenv/testUsdImagingGLHdEmbree
        DEST testUsdImagingGLHdEmbree
    )

    pxr_install_test_dir(
        SRC testenv/testUsdImagingGLHdEmbreePickAndHighlight
        DEST testUsdImagingGLHdEmbreePickAndHighlight
    )

    pxr_register_test(testUsdImagingGLHdEmbreePickAndHighlight
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLPickAndHighlight -renderer HdEmbreeRendererPlugin -offscreen -frameAll -stage test.usda -write testUsdImagingGLHdEmbreePickAndHighlight.png"
        IMAGE_DIFF_COMPARE
            testUsdImagingGLHdEmbreePickAndHighlight_000.png
            testUsdImagingGLHdEmbreePickAndHighlight_001.png
            testUsdImagingGLHdEmbreePickAndHighlight_002.png
            testUsdImagingGLHdEmbreePickAndHighlight_003.png
            testUsdImagingGLHdEmbreePickAndHighlight_004.png
        FAIL_PERCENT 1
        PERCEPTUAL
        EXPECTED_RETURN_CODE 0
        TESTENV testUsdImagingGLHdEmbreePickAndHighlight
        ENV
            HDEMBREE_JITTER_CAMERA=0
            HDEMBREE_AMBIENT_OCCLUSION_SAMPLES=0
            HDEMBREE_SAMPLES_TO_CONVERGENCE=1
    )

    pxr_register_test(testUsdImagingGLHdEmbree
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -shading smooth -frameAll -renderer HdEmbreeRendererPlugin -stage primitives.usda -write default.png"
        IMAGE_DIFF_COMPARE
            default.png
        FAIL_PERCENT 1
        PERCEPTUAL
        EXPECTED_RETURN_CODE 0
        TESTENV testUsdImagingGLHdEmbree
        ENV
            HDEMBREE_JITTER_CAMERA=0
            HDEMBREE_AMBIENT_OCCLUSION_SAMPLES=0
            HDEMBREE_SAMPLES_TO_CONVERGENCE=1
    )

    pxr_register_test(testUsdImagingGLHdEmbree_refined
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -lighting -shading smooth -frameAll -complexity 1.1 -renderer HdEmbreeRendererPlugin -stage primitives.usda -write refined.png"
        IMAGE_DIFF_COMPARE
            refined.png
        FAIL_PERCENT 1
        PERCEPTUAL
        EXPECTED_RETURN_CODE 0
        TESTENV testUsdImagingGLHdEmbree
        ENV
            HDEMBREE_JITTER_CAMERA=0
            HDEMBREE_AMBIENT_OCCLUSION_SAMPLES=0
            HDEMBREE_SAMPLES_TO_CONVERGENCE=1
    )

    pxr_register_test(testUsdImagingGLFramingEmbree_1
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -renderer HdEmbreeRendererPlugin -rendererAov color -offscreen -stage framing.usda -clear 0 0 0 1 -lighting -camera /main_cam -dataWindow 0 0 640 480 -displayWindow 0 0 640 480 -windowPolicy fit -write testUsdImagingGLFramingEmbree1.png"
        IMAGE_DIFF_COMPARE
            testUsdImagingGLFramingEmbree1.png
        FAIL 1
        FAIL_PERCENT 0.1
        WARN 0.02
        WARN_PERCENT 0.02
        PERCEPTUAL
        EXPECTED_RETURN_CODE 0
        TESTENV testUsdImagingGLFraming
        ENV
            HDEMBREE_JITTER_CAMERA=0
            HDEMBREE_AMBIENT_OCCLUSION_SAMPLES=0
            HDEMBREE_SAMPLES_TO_CONVERGENCE=1
    )


    pxr_register_test(testUsdImagingGLFramingEmbree_2
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -renderer HdEmbreeRendererPlugin -rendererAov color -offscreen -stage framing.usda -clear 0 0 0 1 -lighting -camera /main_cam -dataWindow 0 40 640 430 -displayWindow 0 0 1280 960 -windowPolicy fit -write testUsdImagingGLFramingEmbree2.png"
        IMAGE_DIFF_COMPARE
            testUsdImagingGLFramingEmbree2.png
        FAIL 1
        FAIL_PERCENT 0.1
        WARN 0.02
        WARN_PERCENT 0.02
        PERCEPTUAL
        EXPECTED_RETURN_CODE 0
        TESTENV testUsdImagingGLFraming
        ENV
            HDEMBREE_JITTER_CAMERA=0
            HDEMBREE_AMBIENT_OCCLUSION_SAMPLES=0
            HDEMBREE_SAMPLES_TO_CONVERGENCE=1
    )


    pxr_register_test(testUsdImagingGLFramingEmbree_3
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -renderer HdEmbreeRendererPlugin -rendererAov color -offscreen -stage framing.usda -clear 0 0 0 1 -lighting -camera /main_cam -dataWindow 0 0 640 480 -displayWindow 0 0 320 480 -windowPolicy fit -write testUsdImagingGLFramingEmbree3.png"
        IMAGE_DIFF_COMPARE
            testUsdImagingGLFramingEmbree3.png
        FAIL 1
        FAIL_PERCENT 0.1
        WARN 0.02
        WARN_PERCENT 0.02
        PERCEPTUAL
        EXPECTED_RETURN_CODE 0
        TESTENV testUsdImagingGLFraming
        ENV
            HDEMBREE_JITTER_CAMERA=0
            HDEMBREE_AMBIENT_OCCLUSION_SAMPLES=0
            HDEMBREE_SAMPLES_TO_CONVERGENCE=1
    )


    pxr_register_test(testUsdImagingGLFramingEmbree_4
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -renderer HdEmbreeRendererPlugin -rendererAov color -offscreen -stage framing.usda -clear 0 0 0 1 -lighting -camera /main_cam -dataWindow 0 0 640 480 -displayWindow 0 0 320 480 -windowPolicy fit -pixelAspectRatio 1.5 -write testUsdImagingGLFramingEmbree4.png"
        IMAGE_DIFF_COMPARE
            testUsdImagingGLFramingEmbree4.png
        FAIL 1
        FAIL_PERCENT 0.1
        WARN 0.02
        WARN_PERCENT 0.02
        PERCEPTUAL
        EXPECTED_RETURN_CODE 0
        TESTENV testUsdImagingGLFraming
        ENV
            HDEMBREE_JITTER_CAMERA=0
            HDEMBREE_AMBIENT_OCCLUSION_SAMPLES=0
            HDEMBREE_SAMPLES_TO_CONVERGENCE=1
    )


    pxr_register_test(testUsdImagingGLFramingEmbree_5
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -renderer HdEmbreeRendererPlugin -rendererAov color -offscreen -stage framing.usda -clear 0 0 0 1 -lighting -camera /main_cam -dataWindow 25 136 367 298 -displayWindow 50 80 320 400 -windowPolicy fit -write testUsdImagingGLFramingEmbree5.png"
        IMAGE_DIFF_COMPARE
            testUsdImagingGLFramingEmbree5.png
        FAIL 1
        FAIL_PERCENT 0.1
        WARN 0.02
        WARN_PERCENT 0.02
        PERCEPTUAL
        EXPECTED_RETURN_CODE 0
        TESTENV testUsdImagingGLFraming
        ENV
            HDEMBREE_JITTER_CAMERA=0
            HDEMBREE_AMBIENT_OCCLUSION_SAMPLES=0
            HDEMBREE_SAMPLES_TO_CONVERGENCE=1
    )


    pxr_register_test(testUsdImagingGLFramingEmbree_6
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -renderer HdEmbreeRendererPlugin -rendererAov color -offscreen -stage framing.usda -clear 0 0 0 1 -lighting -camera /OrthoCam -dataWindow 25 136 367 298 -displayWindow 50 80 320 400 -windowPolicy fit -write testUsdImagingGLFramingEmbree6.png"
        IMAGE_DIFF_COMPARE
            testUsdImagingGLFramingEmbree6.png
        FAIL 1
        FAIL_PERCENT 0.1
        WARN 0.02
        WARN_PERCENT 0.02
        PERCEPTUAL
        EXPECTED_RETURN_CODE 0
        TESTENV testUsdImagingGLFraming
        ENV
            HDEMBREE_JITTER_CAMERA=0
            HDEMBREE_AMBIENT_OCCLUSION_SAMPLES=0
            HDEMBREE_SAMPLES_TO_CONVERGENCE=1
    )

    pxr_register_test(testUsdImagingGLAovVisualization_Embree_depth
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -stage viz_aov.usda -renderer HdEmbreeRendererPlugin -rendererAov depth -clear 0 0 0 1 -lighting -camera /main_cam -write testUsdImagingGLAovVisualization_Embree_depth.png"
        IMAGE_DIFF_COMPARE
            testUsdImagingGLAovVisualization_Embree_depth.png
        FAIL 0.05
        FAIL_PERCENT 0.025
        WARN 0.025
        WARN_PERCENT 0.005
        PERCEPTUAL
        EXPECTED_RETURN_CODE 0
        TESTENV testUsdImagingGLAovVisualization
        ENV
            HDEMBREE_JITTER_CAMERA=0
            HDEMBREE_AMBIENT_OCCLUSION_SAMPLES=0
            HDEMBREE_SAMPLES_TO_CONVERGENCE=1
    )


    pxr_register_test(testUsdImagingGLAovVisualization_Embree_primId
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -stage viz_aov.usda -renderer HdEmbreeRendererPlugin -rendererAov primId -clear 0 0 0 1 -lighting -camera /main_cam -write testUsdImagingGLAovVisualization_Embree_primId.png"
        IMAGE_DIFF_COMPARE
            testUsdImagingGLAovVisualization_Embree_primId.png
        FAIL 0.05
        FAIL_PERCENT 0.025
        WARN 0.025
        WARN_PERCENT 0.005
        EXPECTED_RETURN_CODE 0
        TESTENV testUsdImagingGLAovVisualization
        ENV
            HDEMBREE_JITTER_CAMERA=0
            HDEMBREE_AMBIENT_OCCLUSION_SAMPLES=0
            HDEMBREE_SAMPLES_TO_CONVERGENCE=1
    )


    pxr_register_test(testUsdImagingGLAovVisualization_Embree_normal
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -stage viz_aov.usda -renderer HdEmbreeRendererPlugin -rendererAov normal -clear 0 0 0 1 -lighting -camera /main_cam -write testUsdImagingGLAovVisualization_Embree_normal.png"
        IMAGE_DIFF_COMPARE
            testUsdImagingGLAovVisualization_Embree_normal.png
        FAIL 0.05
        FAIL_PERCENT 0.025
        WARN 0.025
        WARN_PERCENT 0.005
        EXPECTED_RETURN_CODE 0
        TESTENV testUsdImagingGLAovVisualization
        ENV
            HDEMBREE_JITTER_CAMERA=0
            HDEMBREE_AMBIENT_OCCLUSION_SAMPLES=0
            HDEMBREE_SAMPLES_TO_CONVERGENCE=1
    )
endif() # Embree tests

#
# Conditionally install and register tests that rely on PTEX.
#
if (PTEX_FOUND AND ${PXR_ENABLE_PTEX_SUPPORT})
    pxr_register_test(testUsdImagingGLBasicDrawing_shaders
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -lighting -offscreen -stage basicDrawing/shaders.usd -frameAll -write testUsdImagingGLBasicDrawing_shaders.png"
        IMAGE_DIFF_COMPARE
            testUsdImagingGLBasicDrawing_shaders.png
        FAIL 1
        FAIL_PERCENT 1
        PERCEPTUAL
        EXPECTED_RETURN_CODE 0
        TESTENV testUsdImagingGLBasicDrawing
        ENV
            HGIGL_ENABLE_BINDLESS_TEXTURE=1
    )

    pxr_register_test(testUsdImagingGLBasicDrawing_shadersBindings
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -lighting -offscreen -stage basicDrawing/shaders_bindings.usd -frameAll -write testUsdImagingGLBasicDrawing_shadersBindings.png"
        IMAGE_DIFF_COMPARE
            testUsdImagingGLBasicDrawing_shadersBindings.png
        FAIL 1
        FAIL_PERCENT 1
        PERCEPTUAL
        EXPECTED_RETURN_CODE 0
        TESTENV testUsdImagingGLBasicDrawing
        ENV
            HGIGL_ENABLE_BINDLESS_TEXTURE=1
    )

    pxr_register_test(testUsdImagingGLBasicDrawing_shadersRelative
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -lighting -offscreen -stage basicDrawing/shaders-relative.usd -frameAll -write testUsdImagingGLBasicDrawing_shadersRelative.png"
        IMAGE_DIFF_COMPARE
            testUsdImagingGLBasicDrawing_shadersRelative.png
        FAIL 1
        FAIL_PERCENT 1
        PERCEPTUAL
        EXPECTED_RETURN_CODE 0
        TESTENV testUsdImagingGLBasicDrawing
        ENV
            HGIGL_ENABLE_BINDLESS_TEXTURE=1
    )

    pxr_register_test(testUsdImagingGLBasicDrawing_shadersAnim
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -lighting -offscreen -times 1 2 3 -stage basicDrawing/shaders_anim.usd -frameAll -write testUsdImagingGLBasicDrawing_shadersAnim.png"
        IMAGE_DIFF_COMPARE
            testUsdImagingGLBasicDrawing_shadersAnim_001.png
            testUsdImagingGLBasicDrawing_shadersAnim_002.png
            testUsdImagingGLBasicDrawing_shadersAnim_003.png
        FAIL 1
        FAIL_PERCENT 1
        PERCEPTUAL
        EXPECTED_RETURN_CODE 0
        TESTENV testUsdImagingGLBasicDrawing
        ENV
            HGIGL_ENABLE_BINDLESS_TEXTURE=1
    )

    pxr_register_test(testUsdImagingGLGeomSubsets_Adaptive
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -complexity 1.1 -offscreen -lighting -camlight -shading wireOnSurface -frameAll -stage testUsdImagingGLGeomSubsets.usda -write testUsdImagingGLGeomSubsets_Adaptive.png"
        IMAGE_DIFF_COMPARE
            testUsdImagingGLGeomSubsets_Adaptive.png
        FAIL 1
        FAIL_PERCENT 1
        PERCEPTUAL
        EXPECTED_RETURN_CODE 0
        TESTENV testUsdImagingGLGeomSubsets
        ENV
            HD_ENABLE_OPENSUBDIV3_ADAPTIVE=1
    )

    pxr_register_test(testUsdImagingGLGeomSubsets_High
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -complexity 1.2 -offscreen -lighting -camlight -shading wireOnSurface -frameAll -stage testUsdImagingGLGeomSubsets.usda -write testUsdImagingGLGeomSubsets_High.png"
        IMAGE_DIFF_COMPARE
            testUsdImagingGLGeomSubsets_High.png
        FAIL 1
        FAIL_PERCENT 1
        PERCEPTUAL
        EXPECTED_RETURN_CODE 0
        TESTENV testUsdImagingGLGeomSubsets
    )


    pxr_register_test(testUsdImagingGLGeomSubsets_VeryHigh
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -complexity 1.3 -offscreen -lighting -camlight -shading wireOnSurface -frameAll -stage testUsdImagingGLGeomSubsets.usda -write testUsdImagingGLGeomSubsets_VeryHigh.png"
        IMAGE_DIFF_COMPARE
            testUsdImagingGLGeomSubsets_VeryHigh.png
        FAIL 1
        FAIL_PERCENT 1
        PERCEPTUAL
        EXPECTED_RETURN_CODE 0
        TESTENV testUsdImagingGLGeomSubsets
    )

    pxr_register_test(testUsdImagingGLInstancing_instance
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -frameAll -lighting -stage instance.usda -write testUsdImagingGLInstancing_instance.png"
        IMAGE_DIFF_COMPARE
            testUsdImagingGLInstancing_instance.png
        FAIL 0.01
        FAIL_PERCENT 0.005
        WARN 0.02
        WARN_PERCENT 0.0025
        EXPECTED_RETURN_CODE 0
        TESTENV testUsdImagingGLInstancing
    )

    pxr_register_test(testUsdImagingGLInstancing_instance2
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -frameAll -lighting -stage instance2.usda -write testUsdImagingGLInstancing_instance2.png"
        IMAGE_DIFF_COMPARE
            testUsdImagingGLInstancing_instance2.png
        FAIL 0.01
        FAIL_PERCENT 0.005
        WARN 0.02
        WARN_PERCENT 0.0025
        EXPECTED_RETURN_CODE 0
        TESTENV testUsdImagingGLInstancing
    )

    pxr_register_test(testUsdImagingGLInstancing_instance3
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -frameAll -lighting -stage instance3.usda -write testUsdImagingGLInstancing_instance3.png"
        IMAGE_DIFF_COMPARE
            testUsdImagingGLInstancing_instance3.png
        FAIL 0.02
        FAIL_PERCENT 0.005
        WARN 0.02
        WARN_PERCENT 0.0025
        EXPECTED_RETURN_CODE 0
        TESTENV testUsdImagingGLInstancing
    )

    pxr_register_test(testUsdImagingGLInstancing_nestedInstance
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -frameAll -lighting -stage nestedInstance.usda -write testUsdImagingGLInstancing_nestedInstance.png"
        IMAGE_DIFF_COMPARE
            testUsdImagingGLInstancing_nestedInstance.png
        FAIL 0.02
        FAIL_PERCENT 0.005
        WARN 0.02
        WARN_PERCENT 0.0025
        EXPECTED_RETURN_CODE 0
        TESTENV testUsdImagingGLInstancing
    )

    pxr_register_test(testUsdImagingGLInstancing_SceneIndex_instance
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -frameAll -lighting -stage instance.usda -write testUsdImagingGLInstancing_instance.png"
        IMAGE_DIFF_COMPARE
            testUsdImagingGLInstancing_instance.png
        FAIL 0.01
        FAIL_PERCENT 0.005
        WARN 0.02
        WARN_PERCENT 0.0025
        EXPECTED_RETURN_CODE 0
        TESTENV testUsdImagingGLInstancing_SceneIndex
    )

    pxr_register_test(testUsdImagingGLInstancing_SceneIndex_instance2
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -frameAll -lighting -stage instance2.usda -write testUsdImagingGLInstancing_instance2.png"
        IMAGE_DIFF_COMPARE
            testUsdImagingGLInstancing_instance2.png
        FAIL 0.01
        FAIL_PERCENT 0.005
        WARN 0.02
        WARN_PERCENT 0.0025
        EXPECTED_RETURN_CODE 0
        TESTENV testUsdImagingGLInstancing_SceneIndex
    )

    pxr_register_test(testUsdImagingGLInstancing_SceneIndex_instance3
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -frameAll -lighting -stage instance3.usda -write testUsdImagingGLInstancing_instance3.png"
        IMAGE_DIFF_COMPARE
            testUsdImagingGLInstancing_instance3.png
        FAIL 0.02
        FAIL_PERCENT 0.005
        WARN 0.02
        WARN_PERCENT 0.0025
        EXPECTED_RETURN_CODE 0
        TESTENV testUsdImagingGLInstancing_SceneIndex
    )

    pxr_register_test(testUsdImagingGLInstancing_SceneIndex_nestedInstance
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -offscreen -frameAll -lighting -stage nestedInstance.usda -write testUsdImagingGLInstancing_nestedInstance.png"
        IMAGE_DIFF_COMPARE
            testUsdImagingGLInstancing_nestedInstance.png
        FAIL 0.02
        FAIL_PERCENT 0.005
        WARN 0.02
        WARN_PERCENT 0.0025
        EXPECTED_RETURN_CODE 0
        TESTENV testUsdImagingGLInstancing_SceneIndex
        ENV
            USDIMAGINGGL_ENGINE_ENABLE_SCENE_INDEX=true
    )

    pxr_register_test(testUsdImagingGLBasicDrawingNonBindless_shaders
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -lighting -offscreen -stage shaders.usd -frameAll -write testUsdImagingGLBasicDrawingNonBindless_shaders.png"
        IMAGE_DIFF_COMPARE
            testUsdImagingGLBasicDrawingNonBindless_shaders.png
        FAIL 1
        FAIL_PERCENT 1
        PERCEPTUAL
        EXPECTED_RETURN_CODE 0
        TESTENV testUsdImagingGLBasicDrawingNonBindless
        ENV
           HGIGL_ENABLE_BINDLESS_TEXTURE=0
    )

    pxr_register_test(testUsdImagingGLBasicDrawingNonBindless_shadersRelative
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -lighting -offscreen -stage shaders-relative.usd -frameAll -write testUsdImagingGLBasicDrawingNonBindless_shadersRelative.png"
        IMAGE_DIFF_COMPARE
            testUsdImagingGLBasicDrawingNonBindless_shadersRelative.png
        FAIL 1
        FAIL_PERCENT 1
        PERCEPTUAL
        EXPECTED_RETURN_CODE 0
        TESTENV testUsdImagingGLBasicDrawingNonBindless
        ENV
            HGIGL_ENABLE_BINDLESS_TEXTURE=0
    )
endif() # PTEX tests

#
# Conditionally install and register tests that rely on OpenVDB.
#
if (OPENVDB_FOUND AND ${PXR_ENABLE_OPENVDB_SUPPORT})
    pxr_install_test_dir(
        SRC testenv/testUsdImagingGLVdbVolume
        DEST testUsdImagingGLVdbVolume
    )
    
    pxr_register_test(testUsdImagingGLVdbVolume
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -rendererAov color -offscreen -stage vdbVolume.usda -clear 0 0 0 1 -lighting -camera /main_cam -write testUsdImagingGLVdbVolume.png"
        IMAGE_DIFF_COMPARE
            testUsdImagingGLVdbVolume.png
        FAIL 1
        FAIL_PERCENT 1
        PERCEPTUAL
        EXPECTED_RETURN_CODE 0
        TESTENV testUsdImagingGLVdbVolume
        ENV
            HGIGL_ENABLE_BINDLESS_TEXTURE=1
    )


    pxr_register_test(testUsdImagingGLVdbVolume_DisableSceneMaterials
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -disableSceneMaterials -rendererAov color -offscreen -stage vdbVolume.usda -clear 0 0 0 1 -lighting -camera /main_cam -write testUsdImagingGLVdbVolumeDisableSceneMaterials.png"
        IMAGE_DIFF_COMPARE
            testUsdImagingGLVdbVolumeDisableSceneMaterials.png
        FAIL 1
        FAIL_PERCENT 1
        PERCEPTUAL
        EXPECTED_RETURN_CODE 0
        TESTENV testUsdImagingGLVdbVolume
        ENV
            HGIGL_ENABLE_BINDLESS_TEXTURE=1
    )


    pxr_register_test(testUsdImagingGLVdbVolume_Large
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -rendererAov color -offscreen -stage vdbVolumeLarge.usda -clear 0 0 0 1 -lighting -camera /World/main_cam -write testUsdImagingGLVdbVolumeLarge.png"
        IMAGE_DIFF_COMPARE
            testUsdImagingGLVdbVolumeLarge.png
        FAIL 1
        FAIL_PERCENT 1
        PERCEPTUAL
        EXPECTED_RETURN_CODE 0
        TESTENV testUsdImagingGLVdbVolume
        ENV
            HGIGL_ENABLE_BINDLESS_TEXTURE=1
    )


    pxr_register_test(testUsdImagingGLVdbVolume_Flipped
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -rendererAov color -offscreen -stage vdbVolumeFlipped.usda -clear 0 0 0 1 -lighting -camera /main_cam -write testUsdImagingGLVdbVolumeFlipped.png"
        IMAGE_DIFF_COMPARE
            testUsdImagingGLVdbVolumeFlipped.png
        FAIL 1
        FAIL_PERCENT 1
        PERCEPTUAL
        EXPECTED_RETURN_CODE 0
        TESTENV testUsdImagingGLVdbVolume
        ENV
            HGIGL_ENABLE_BINDLESS_TEXTURE=1
    )


    pxr_register_test(testUsdImagingGLVdbVolume_FallbackMaterial
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -rendererAov color -offscreen -stage vdbVolumeFallbackMaterial.usda -times 1 2 3 -clear 0 0 0 1 -lighting -camera /main_cam -write testUsdImagingGLVdbVolumeFallbackMaterial.png"
        IMAGE_DIFF_COMPARE
            testUsdImagingGLVdbVolumeFallbackMaterial_001.png
            testUsdImagingGLVdbVolumeFallbackMaterial_002.png
            testUsdImagingGLVdbVolumeFallbackMaterial_003.png
        FAIL 1
        FAIL_PERCENT 1
        PERCEPTUAL
        EXPECTED_RETURN_CODE 0
        TESTENV testUsdImagingGLVdbVolume
        ENV
            HGIGL_ENABLE_BINDLESS_TEXTURE=1
    )


    pxr_register_test(testUsdImagingGLVdbVolume_Inside
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -rendererAov color -offscreen -stage vdbVolumeInside.usda -times 1 13 -clear 0 0 0 1 -lighting -camera /main_cam -write testUsdImagingGLVdbVolumeInside.png"
        IMAGE_DIFF_COMPARE
            testUsdImagingGLVdbVolumeInside_001.png
            testUsdImagingGLVdbVolumeInside_013.png
        FAIL 0.005
        FAIL_PERCENT 0.001
        WARN 0.005
        WARN_PERCENT 0.0005
        EXPECTED_RETURN_CODE 0
        TESTENV testUsdImagingGLVdbVolume
        ENV
            HGIGL_ENABLE_BINDLESS_TEXTURE=1
    )


    pxr_register_test(testUsdImagingGLVdbVolume_Ortho
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -rendererAov color -offscreen -stage vdbVolumeOrtho.usda -clear 0 0 0 1 -lighting -camera /main_cam -write testUsdImagingGLVdbVolumeOrtho.png"
        IMAGE_DIFF_COMPARE
            testUsdImagingGLVdbVolumeOrtho.png
        FAIL_PERCENT 2
        PERCEPTUAL
        EXPECTED_RETURN_CODE 0
        TESTENV testUsdImagingGLVdbVolume
        ENV
            HGIGL_ENABLE_BINDLESS_TEXTURE=1
    )


    pxr_register_test(testUsdImagingGLVdbVolume_TwoFields
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -rendererAov color -offscreen -stage vdbVolumeTwoFields.usda -clear 0 0 0 1 -lighting -camera /main_cam -write testUsdImagingGLVdbVolumeTwoFields.png"
        IMAGE_DIFF_COMPARE
            testUsdImagingGLVdbVolumeTwoFields.png
        FAIL_PERCENT 2
        PERCEPTUAL
        EXPECTED_RETURN_CODE 0
        TESTENV testUsdImagingGLVdbVolume
        ENV
            HGIGL_ENABLE_BINDLESS_TEXTURE=1
    )


    pxr_register_test(testUsdImagingGLVdbVolume_TargetMemory
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -rendererAov color -offscreen -stage vdbVolumeTargetMemory.usda -times 2 -clear 0 0 0 1 -lighting -camera /main_cam -write testUsdImagingGLVdbVolumeTargetMemory.png"
        IMAGE_DIFF_COMPARE
            testUsdImagingGLVdbVolumeTargetMemory_002.png
        FAIL 1
        FAIL_PERCENT 1
        PERCEPTUAL
        EXPECTED_RETURN_CODE 0
        TESTENV testUsdImagingGLVdbVolume
        ENV
            HGIGL_ENABLE_BINDLESS_TEXTURE=1
    )

    pxr_register_test(testUsdImagingGLVdbVolumeNonBindless
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -rendererAov color -offscreen -stage vdbVolume.usda -clear 0 0 0 1 -lighting -camera /main_cam -write testUsdImagingGLVdbVolume.png"
        IMAGE_DIFF_COMPARE
            testUsdImagingGLVdbVolume.png
        FAIL 1
        FAIL_PERCENT 1
        PERCEPTUAL
        EXPECTED_RETURN_CODE 0
        TESTENV testUsdImagingGLVdbVolume
        ENV
            HGIGL_ENABLE_BINDLESS_TEXTURE=0
    )


    pxr_register_test(testUsdImagingGLVdbVolumeNonBindless_DisableSceneMaterials
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -disableSceneMaterials -rendererAov color -offscreen -stage vdbVolume.usda -clear 0 0 0 1 -lighting -camera /main_cam -write testUsdImagingGLVdbVolumeDisableSceneMaterials.png"
        IMAGE_DIFF_COMPARE
            testUsdImagingGLVdbVolumeDisableSceneMaterials.png
        FAIL 1
        FAIL_PERCENT 1
        PERCEPTUAL
        EXPECTED_RETURN_CODE 0
        TESTENV testUsdImagingGLVdbVolume
        ENV
            HGIGL_ENABLE_BINDLESS_TEXTURE=0
    )


    pxr_register_test(testUsdImagingGLVdbVolumeNonBindless_Large
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -rendererAov color -offscreen -stage vdbVolumeLarge.usda -clear 0 0 0 1 -lighting -camera /World/main_cam -write testUsdImagingGLVdbVolumeLarge.png"
        IMAGE_DIFF_COMPARE
            testUsdImagingGLVdbVolumeLarge.png
        FAIL 1
        FAIL_PERCENT 1
        PERCEPTUAL
        EXPECTED_RETURN_CODE 0
        TESTENV testUsdImagingGLVdbVolume
        ENV
            HGIGL_ENABLE_BINDLESS_TEXTURE=0
    )


    pxr_register_test(testUsdImagingGLVdbVolumeNonBindless_Flipped
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -rendererAov color -offscreen -stage vdbVolumeFlipped.usda -clear 0 0 0 1 -lighting -camera /main_cam -write testUsdImagingGLVdbVolumeFlipped.png"
        IMAGE_DIFF_COMPARE
            testUsdImagingGLVdbVolumeFlipped.png
        FAIL 1
        FAIL_PERCENT 1
        PERCEPTUAL
        EXPECTED_RETURN_CODE 0
        TESTENV testUsdImagingGLVdbVolume
        ENV
            HGIGL_ENABLE_BINDLESS_TEXTURE=0
    )


    pxr_register_test(testUsdImagingGLVdbVolumeNonBindless_FallbackMaterial
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -rendererAov color -offscreen -stage vdbVolumeFallbackMaterial.usda -times 1 2 3 -clear 0 0 0 1 -lighting -camera /main_cam -write testUsdImagingGLVdbVolumeFallbackMaterial.png"
        IMAGE_DIFF_COMPARE
            testUsdImagingGLVdbVolumeFallbackMaterial_001.png
            testUsdImagingGLVdbVolumeFallbackMaterial_002.png
            testUsdImagingGLVdbVolumeFallbackMaterial_003.png
        FAIL 1
        FAIL_PERCENT 1
        PERCEPTUAL
        EXPECTED_RETURN_CODE 0
        TESTENV testUsdImagingGLVdbVolume
        ENV
            HGIGL_ENABLE_BINDLESS_TEXTURE=0
    )


    pxr_register_test(testUsdImagingGLVdbVolumeNonBindless_Inside
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -rendererAov color -offscreen -stage vdbVolumeInside.usda -times 1 13 -clear 0 0 0 1 -lighting -camera /main_cam -write testUsdImagingGLVdbVolumeInside.png"
        IMAGE_DIFF_COMPARE
            testUsdImagingGLVdbVolumeInside_001.png
            testUsdImagingGLVdbVolumeInside_013.png
        FAIL 0.005
        FAIL_PERCENT 0.001
        WARN 0.005
        WARN_PERCENT 0.0005
        EXPECTED_RETURN_CODE 0
        TESTENV testUsdImagingGLVdbVolume
        ENV
            HGIGL_ENABLE_BINDLESS_TEXTURE=0
    )


    pxr_register_test(testUsdImagingGLVdbVolumeNonBindless_Ortho
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -rendererAov color -offscreen -stage vdbVolumeOrtho.usda -clear 0 0 0 1 -lighting -camera /main_cam -write testUsdImagingGLVdbVolumeOrtho.png"
        IMAGE_DIFF_COMPARE
            testUsdImagingGLVdbVolumeOrtho.png
        FAIL_PERCENT 2
        PERCEPTUAL
        EXPECTED_RETURN_CODE 0
        TESTENV testUsdImagingGLVdbVolume
        ENV
            HGIGL_ENABLE_BINDLESS_TEXTURE=0
    )


    pxr_register_test(testUsdImagingGLVdbVolumeNonBindless_TwoFields
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -rendererAov color -offscreen -stage vdbVolumeTwoFields.usda -clear 0 0 0 1 -lighting -camera /main_cam -write testUsdImagingGLVdbVolumeTwoFields.png"
        IMAGE_DIFF_COMPARE
            testUsdImagingGLVdbVolumeTwoFields.png
        FAIL_PERCENT 2
        PERCEPTUAL
        EXPECTED_RETURN_CODE 0
        TESTENV testUsdImagingGLVdbVolume
        ENV
            HGIGL_ENABLE_BINDLESS_TEXTURE=0
    )


    pxr_register_test(testUsdImagingGLVdbVolumeNonBindless_TargetMemory
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingGLBasicDrawing -rendererAov color -offscreen -stage vdbVolumeTargetMemory.usda -times 2 -clear 0 0 0 1 -lighting -camera /main_cam -write testUsdImagingGLVdbVolumeTargetMemory.png"
        IMAGE_DIFF_COMPARE
            testUsdImagingGLVdbVolumeTargetMemory_002.png
        FAIL 1
        FAIL_PERCENT 1
        PERCEPTUAL
        EXPECTED_RETURN_CODE 0
        TESTENV testUsdImagingGLVdbVolume
        ENV
            HGIGL_ENABLE_BINDLESS_TEXTURE=0
    )
endif()
