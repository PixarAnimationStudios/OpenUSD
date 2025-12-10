set(PXR_PREFIX pxr/usdImaging)
set(PXR_PACKAGE usdImaging)

pxr_library(usdImaging
    LIBRARIES
        gf
        tf
        plug
        trace
        vt
        work
        geomUtil
        hd
        hdar
        hio
        pxOsd
        sdf
        usd
        usdGeom
        usdLux
        usdRender
        usdShade
        usdVol
        ar
        TBB::tbb

    PUBLIC_CLASSES
        adapterRegistry
        collectionCache
        collectionMaterialBindingSchema
        directMaterialBindingSchema
        dataSourceAttribute
        dataSourceAttributeColorSpace
        dataSourceAttributeTypeName
        dataSourceBasisCurves
        dataSourceCamera
        dataSourceGprim
        dataSourceMapped
        dataSourceMaterial
        dataSourceMesh
        dataSourceNurbsCurves
        dataSourceNurbsPatch
        dataSourcePointInstancer
        dataSourcePoints
        dataSourcePrim
        dataSourcePrimvars
        dataSourceRelationship
        dataSourceRenderPrims
        dataSourceStage
        dataSourceStageGlobals
        dataSourceTetMesh
        dataSourceUsdPrimInfo
        dataSourceVolume
        debugCodes
        delegate
        drawModeAdapter
        drawModeSceneIndex
        extentResolvingSceneIndex
        extentsHintSchema
        geomModelSchema
        geomXformVectorsSchema
        geomSubsetAdapter
        indexProxy
        materialParamUtils
        modelSchema
        primvarUtils
        prototypeSceneIndexUtils
        resolvedAttributeCache
        instancerContext
        stageSceneIndex
        selectionSceneIndex
        tokens
        apiSchemaAdapter
        basisCurvesAdapter
        capsuleAdapter
        cameraAdapter
        collectionAPIAdapter
        coneAdapter
        coordSysAdapter
        coordSysAPIAdapter
        cubeAdapter
        cylinderAdapter
        cylinderLightAdapter
        diskLightAdapter
        distantLightAdapter
        domeLightAdapter
        domeLight_1Adapter
        fieldAdapter
        flattenedDataSourceProviders
        flattenedGeomModelDataSourceProvider
        flattenedMaterialBindingsDataSourceProvider
        geometryLightAdapter
        geomModelAPIAdapter
        gprimAdapter
        hermiteCurvesAdapter
        implicitSurfaceMeshUtils
        instanceablePrimAdapter
        instanceAdapter
        lightAdapter
        lightAPIAdapter
        lightFilterAdapter
        materialAdapter
        materialBindingAPIAdapter
        materialBindingSchema
        materialBindingsSchema
        materialBindingsResolvingSceneIndex
        meshAdapter
        niPrototypePropagatingSceneIndex
        nurbsCurvesAdapter
        nurbsPatchAdapter
        piPrototypePropagatingSceneIndex
        planeAdapter
        pluginLightAdapter
        pluginLightFilterAdapter
        pointsAdapter
        pointInstancerAdapter
        portalLightAdapter
        primAdapter
        primvarDescCache
        rootOverridesSceneIndex
        unloadedDrawModeSceneIndex
        usdPrimInfoSchema
        usdRenderProductSchema
        usdRenderSettingsSchema
        usdRenderVarSchema
        rectLightAdapter
        renderPassAdapter
        renderProductAdapter
        renderSettingsAdapter
        renderSettingsFlatteningSceneIndex
        renderVarAdapter
        representedByAncestorPrimAdapter
        rerootingContainerDataSource
        rerootingSceneIndex
        sceneIndexPlugin
        sceneIndexPrimAdapter
        sceneIndices
        sphereAdapter
        sphereLightAdapter
        tetMeshAdapter
        textureUtils
        volumeAdapter

    PRIVATE_CLASSES
        adapterManager
        drawModeStandin
        instanceProxyPathTranslationSceneIndex
        niInstanceAggregationSceneIndex
        niPrototypePruningSceneIndex
        niPrototypeSceneIndex
        piPrototypeSceneIndex

    PUBLIC_HEADERS
        api.h
        types.h
        unitTestHelper.h
        version.h

    PRIVATE_HEADERS
        dataSourceImplicits-Impl.h

    RESOURCE_FILES
        plugInfo.json
)

# Build tests
pxr_build_test(testUsdImagingDataSourceAttribute
    LIBRARIES
        usdImaging
        usd
        sdf
        hd
        tf
    CPPFILES
        testenv/testUsdImagingDataSourceAttribute.cpp
)
pxr_build_test(testUsdImagingDelegate
    LIBRARIES
        usdImaging
        usdGeom
        usd
        hd
        tf
        gf
        arch
    CPPFILES
        testenv/testUsdImagingDelegate.cpp
)
pxr_build_test(testUsdImagingDelegateChanges
    LIBRARIES
        usdImaging
        usdShade
        usdGeom
        usd
        sdf
        hd
        tf
        gf
        arch
    CPPFILES
        testenv/testUsdImagingDelegateChanges.cpp
)
pxr_build_test(testUsdImagingExcluded
    LIBRARIES
        usdImaging
        usdGeom
        usd
        hd
        gf
        arch
    CPPFILES
        testenv/testUsdImagingExcluded.cpp
)
pxr_build_test(testUsdImagingMaterialBinding
    LIBRARIES
        usdImaging
        usdShade
        usd
        hd
        garch
        tf
        gf
        vt
    CPPFILES
        testenv/testUsdImagingMaterialBinding.cpp
)
pxr_build_test(testUsdImagingNestedInstancingCategories
    LIBRARIES
        usdImaging
        usd
        tf
    CPPFILES
        testenv/testUsdImagingNestedInstancingCategories.cpp
)
pxr_build_test(testUsdImagingPrimvarUtils
    LIBRARIES
        usdImaging
        usdGeom
        tf
    CPPFILES
        testenv/testUsdImagingPrimvarUtils.cpp
)
pxr_build_test(testUsdImagingRerootingSceneIndex
    LIBRARIES
        usdImaging
        hd
        vt
    CPPFILES
        testenv/testUsdImagingRerootingSceneIndex.cpp
)
pxr_build_test(testUsdImagingRootPrim
    LIBRARIES
        usdImaging
        usdGeom
        usd
        hd
        gf
        arch
    CPPFILES
        testenv/testUsdImagingRootPrim.cpp
)
pxr_build_test(testUsdImagingStageSceneIndex
    LIBRARIES
        usdImaging
        usdGeom
        usd
        sdf
        hd
        tf
    CPPFILES
        testenv/testUsdImagingStageSceneIndex.cpp
)
pxr_build_test(testUsdImagingStageSceneIndexContents
    LIBRARIES
        usdImaging
        hd
    CPPFILES
        testenv/testUsdImagingStageSceneIndexContents.cpp
)
pxr_build_test(testUsdImagingColorSpace
    LIBRARIES
        usdImaging
        hd
    CPPFILES
        testenv/testUsdImagingColorSpace.cpp
)

# Register tests to be run
pxr_register_test(testUsdImagingDataSourceAttribute
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingDataSourceAttribute"
    EXPECTED_RETURN_CODE 0
)
pxr_register_test(testUsdImagingDelegate
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingDelegate"
    TESTENV testenv/testUsdImagingDelegate
    EXPECTED_RETURN_CODE 0
)
pxr_register_test(testUsdImagingDelegateChanges
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingDelegateChanges"
    TESTENV testenv/testUsdImagingDelegateChanges
    EXPECTED_RETURN_CODE 0
)
pxr_register_test(testUsdImagingExcluded
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingExcluded"
    EXPECTED_RETURN_CODE 0
)
# Tests that rely on plugins aren't currently supported in static builds.
if (BUILD_SHARED_LIBS)
    pxr_register_test(testUsdImagingMaterialBinding
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingMaterialBinding"
        TESTENV testenv/testUsdImagingMaterialBinding
        EXPECTED_RETURN_CODE 0
        ENV
            USDIMAGINGGL_ENGINE_ENABLE_SCENE_INDEX=1
    )
    pxr_register_test(testUsdImagingColorSpace
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingColorSpace"
        TESTENV testenv/testUsdImagingColorSpace
        EXPECTED_RETURN_CODE 0
    )
endif()
pxr_register_test(testUsdImagingNestedInstancingCategories
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingNestedInstancingCategories"
    TESTENV testenv/testUsdImagingNestedInstancingCategories
    EXPECTED_RETURN_CODE 0
    ENV
        USD_ASSIGN_PROTOTYPES_DETERMINISTICALLY=1
)
pxr_register_test(testUsdImagingPrimvarUtils
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingPrimvarUtils"
    EXPECTED_RETURN_CODE 0
)
pxr_register_test(testUsdImagingRerootingSceneIndex
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingRerootingSceneIndex"
    EXPECTED_RETURN_CODE 0
)
pxr_register_test(testUsdImagingRootPrim
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingRootPrim"
    EXPECTED_RETURN_CODE 0
)
pxr_register_test(testUsdImagingStageSceneIndex
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingStageSceneIndex"
    TESTENV testenv/testUsdImagingStageSceneIndex
    EXPECTED_RETURN_CODE 0
)
pxr_register_test(testUsdImagingStageSceneIndexContents
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingStageSceneIndexContents basic.usda basic.txt"
    TESTENV testenv/testUsdImagingStageSceneIndexContents
    EXPECTED_RETURN_CODE 0
    DIFF_COMPARE 
        basic.txt
)

# This test requires MaterialX support and is not currently supported on static 
# builds as it uses a plugin mechanism which is not supported for static build 
# configurations. 
if (BUILD_SHARED_LIBS AND ${PXR_ENABLE_MATERIALX_SUPPORT})

pxr_build_test(testUsdImagingHdMtlx
    LIBRARIES
        tf
        hd
        hdMtlx
        usdImaging
    CPPFILES
        testenv/testUsdImagingHdMtlx.cpp
)
pxr_register_test(testUsdImagingHdMtlx_withMtlxVersion
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingHdMtlx --filename withMtlxVersion.usda"
    EXPECTED_RETURN_CODE 0
    DIFF_COMPARE
        withMtlxVersion.mtlx
    TESTENV testenv/testUsdImagingHdMtlx
)
pxr_register_test(testUsdImagingHdMtlx_withoutMtlxVersion
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingHdMtlx --filename withoutMtlxVersion.usda"
    EXPECTED_RETURN_CODE 0
    DIFF_COMPARE
        withoutMtlxVersion.mtlx
    TESTENV testenv/testUsdImagingHdMtlx
)
pxr_register_test(testUsdImagingHdMtlx_atan2Node
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingHdMtlx --filename atan2Node.usda"
    EXPECTED_RETURN_CODE 0
    DIFF_COMPARE
        atan2Node.mtlx
    TESTENV testenv/testUsdImagingHdMtlx
)
pxr_register_test(testUsdImagingHdMtlx_switchNode
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingHdMtlx --filename switchNode.usda"
    EXPECTED_RETURN_CODE 0
    DIFF_COMPARE
        switchNode.mtlx
    TESTENV testenv/testUsdImagingHdMtlx
)
pxr_register_test(testUsdImagingHdMtlx_swizzleNode
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingHdMtlx --filename swizzleNode.usda"
    EXPECTED_RETURN_CODE 0
    DIFF_COMPARE
        swizzleNode.mtlx
    TESTENV testenv/testUsdImagingHdMtlx
)

endif() # BUILD_SHARED_LIBS AND PXR_ENABLE_MATERIALX_SUPPORT
