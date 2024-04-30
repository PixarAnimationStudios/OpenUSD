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
        ${TBB_tbb_LIBRARY}

    INCLUDE_DIRS
        ${TBB_INCLUDE_DIRS}

    PUBLIC_CLASSES
        adapterRegistry
        collectionCache
        collectionMaterialBindingSchema
        collectionMaterialBindingsSchema
        directMaterialBindingSchema
        directMaterialBindingsSchema
        dataSourceAttribute
        dataSourceAttributeColorSpace
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
        geomSubsetAdapter
        indexProxy
        materialParamUtils
        modelSchema
        primvarUtils
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
        flattenedDirectMaterialBindingsDataSourceProvider
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
        rerootingSceneIndex
        sceneIndices
        sphereAdapter
        sphereLightAdapter
        tetMeshAdapter
        textureUtils
        volumeAdapter

    PRIVATE_CLASSES
        adapterManager
        drawModeStandin
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
        glf
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

# Install (copy test assets and baselines)
pxr_install_test_dir(
    SRC testenv/testUsdImagingDelegate
    DEST testUsdImagingDelegate
)
pxr_install_test_dir(
    SRC testenv/testUsdImagingDelegateChanges
    DEST testUsdImagingDelegateChanges
)
pxr_install_test_dir(
    SRC testenv/testUsdImagingMaterialBinding
    DEST testUsdImagingMaterialBinding
)
pxr_install_test_dir(
    SRC testenv/testUsdImagingNestedInstancingCategories
    DEST testUsdImagingNestedInstancingCategories
)
pxr_install_test_dir(
    SRC testenv/testUsdImagingStageSceneIndex
    DEST testUsdImagingStageSceneIndex
)
pxr_install_test_dir(
    SRC testenv/testUsdImagingStageSceneIndexContents
    DEST testUsdImagingStageSceneIndexContents
)

# Register tests to be run
pxr_register_test(testUsdImagingDataSourceAttribute
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingDataSourceAttribute"
    EXPECTED_RETURN_CODE 0
)
pxr_register_test(testUsdImagingDelegate
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingDelegate"
    EXPECTED_RETURN_CODE 0
)
pxr_register_test(testUsdImagingDelegateChanges
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingDelegateChanges"
    EXPECTED_RETURN_CODE 0
)
pxr_register_test(testUsdImagingExcluded
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingExcluded"
    EXPECTED_RETURN_CODE 0
)
pxr_register_test(testUsdImagingMaterialBinding
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingMaterialBinding"
    EXPECTED_RETURN_CODE 0
    ENV
        USDIMAGINGGL_ENGINE_ENABLE_SCENE_INDEX=1
)
pxr_register_test(testUsdImagingNestedInstancingCategories
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingNestedInstancingCategories"
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
    EXPECTED_RETURN_CODE 0
)
pxr_register_test(testUsdImagingStageSceneIndexContents
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdImagingStageSceneIndexContents basic.usda basic.txt"
    EXPECTED_RETURN_CODE 0
    DIFF_COMPARE 
        basic.txt
)
