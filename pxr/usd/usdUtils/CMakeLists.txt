set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usdUtils)

pxr_library(usdUtils
    LIBRARIES
        arch
        tf
        gf
        sdf
        usd
        usdGeom
        usdShade
        ${Boost_PYTHON_LIBRARY}
        ${TBB_tbb_LIBRARY}

    INCLUDE_DIRS
        ${Boost_INCLUDE_DIRS}
        ${TBB_INCLUDE_DIRS}


    PUBLIC_CLASSES
        authoring
        coalescingDiagnosticDelegate
        conditionalAbortDiagnosticDelegate
        debugCodes
        dependencies
        flattenLayerStack
        introspection
        localizeAsset
        pipeline
        registeredVariantSet
        sparseValueWriter
        stageCache
        stitch
        stitchClips
        timeCodeRange
        usdzPackage

    PRIVATE_CLASSES
        assetLocalization
        assetLocalizationDelegate
        assetLocalizationPackage

    PUBLIC_HEADERS
        api.h
        userProcessingFunc.h

    PYTHON_CPPFILES
        moduleDeps.cpp

    PYMODULE_CPPFILES
        module.cpp
        wrapAuthoring.cpp
        wrapCoalescingDiagnosticDelegate.cpp
        wrapConditionalAbortDiagnosticDelegate.cpp
        wrapDependencies.cpp
        wrapFlattenLayerStack.cpp
        wrapIntrospection.cpp
        wrapLocalizeAsset.cpp
        wrapPipeline.cpp
        wrapRegisteredVariantSet.cpp
        wrapSparseValueWriter.cpp
        wrapStageCache.cpp
        wrapStitch.cpp
        wrapStitchClips.cpp
        wrapTimeCodeRange.cpp
        wrapUserProcessingFunc.cpp

    PYMODULE_FILES
        __init__.py
        complianceChecker.py
        constantsGroup.py
        fixBrokenPixarSchemas.py
        toolPaths.py
        updateSchemaWithSdrNode.py
        usdzUtils.py

    DOXYGEN_FILES
        overview.dox
)

pxr_test_scripts(
    testenv/testUsdUtilsAuthoring.py
    testenv/testUsdUtilsCoalescingDiagnosticDelegate.py
    testenv/testUsdUtilsConditionalAbortDiagnosticDelegate.py
    testenv/testUsdUtilsConstantsGroup.py
    testenv/testUsdUtilsCreateNewUsdzPackage.py
    testenv/testUsdUtilsDependencies.py
    testenv/testUsdUtilsDependenciesCustomResolver.py
    testenv/testUsdUtilsDependencyExtractor.py
    testenv/testUsdUtilsFlattenLayerStack.py
    testenv/testUsdUtilsIntrospection.py
    testenv/testUsdUtilsLocalizeAsset.py
    testenv/testUsdUtilsModifyAssetPaths.py
    testenv/testUsdUtilsPipeline.py
    testenv/testUsdUtilsSparseValueWriter.py
    testenv/testUsdUtilsStageCache.py
    testenv/testUsdUtilsStitch.py
    testenv/testUsdUtilsStitchClips.py
    testenv/testUsdUtilsTimeCodeRange.py
    testenv/testUsdUtilsUpdateSchemaWithSdrNode.py
    testenv/testUsdUtilsUsdzUtils.py
    testenv/testUsdUtilsUserProcessingFunc.py
    testenv/testUsdUtilsVarSelsSessionLayer.py
)

pxr_build_test_shared_lib(TestUsdUtilsDependenciesCustomResolver
    INSTALL_PREFIX UsdUtilsPlugins
    LIBRARIES
        ar
        sdf
        tf

    CPPFILES
        testenv/TestUsdUtilsDependenciesCustomResolver_plugin.cpp
)

pxr_build_test(testUsdUtilsCoalescingDiagnosticDelegateCpp
    LIBRARIES
        tf
        usd
        usdUtils
    CPPFILES
        testenv/testUsdUtilsCoalescingDiagnosticDelegate.cpp
)

pxr_build_test(testUsdUtilsStitchCpp
    LIBRARIES
        sdf
        tf
        usd
        usdUtils
    CPPFILES
        testenv/testUsdUtilsStitch.cpp
)

pxr_build_test(testUsdUtilsTimeCodeRangeCpp
    LIBRARIES
        gf
        tf
        usd
        usdUtils
    CPPFILES
        testenv/testUsdUtilsTimeCodeRange.cpp
)

pxr_install_test_dir(
    SRC testenv/testUsdUtilsAuthoring
    DEST testUsdUtilsAuthoring
)

pxr_register_test(testUsdUtilsConstantsGroup
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdUtilsConstantsGroup"
    EXPECTED_RETURN_CODE 0
)

pxr_install_test_dir(
    SRC testenv/testUsdUtilsCreateNewUsdzPackage
    DEST testUsdUtilsCreateNewARKitUsdzPackage
)

pxr_install_test_dir(
    SRC testenv/testUsdUtilsCreateNewUsdzPackage
    DEST testUsdUtilsCreateNewARKitUsdzPackageInPlace
)

pxr_install_test_dir(
    SRC testenv/testUsdUtilsCreateNewUsdzPackage
    DEST testUsdUtilsCreateNewUsdzPackage
)

pxr_install_test_dir(
    SRC testenv/testUsdUtilsCreateNewUsdzPackage
    DEST testUsdUtilsCreateNewUsdzPackageContextDep
)

pxr_install_test_dir(
    SRC testenv/testUsdUtilsCreateNewUsdzPackage
    DEST testUsdUtilsCreateNewUsdzPackageFilterMetadataKeys
)

pxr_install_test_dir(
    SRC testenv/testUsdUtilsCreateNewUsdzPackage
    DEST testUsdUtilsCreateNewUsdzPackageUdim
)

pxr_install_test_dir(
   SRC testenv/testUsdUtilsCreateNewUsdzPackage
   DEST testUsdUtilsCreateNewUsdzPackageRemapClipsDir
)

pxr_install_test_dir(
   SRC testenv/testUsdUtilsCreateNewUsdzPackage
   DEST testUsdUtilsCreateNewUsdzPackageEditInPlace
)

pxr_install_test_dir(
    SRC testenv/testUsdUtilsDependencies
    DEST testUsdUtilsDependencies
)

pxr_install_test_dir(
    SRC testenv/testUsdUtilsDependenciesCustomResolver
    DEST testUsdUtilsDependenciesCustomResolver
)

pxr_install_test_dir(
    SRC testenv/testUsdUtilsDependencyExtractor
    DEST testUsdUtilsDependencyExtractor1
)

pxr_install_test_dir(
    SRC testenv/testUsdUtilsDependencyExtractor
    DEST testUsdUtilsDependencyExtractor2
)

pxr_install_test_dir(
    SRC testenv/testUsdUtilsDependencyExtractor
    DEST testUsdUtilsDependencyExtractor3
)

pxr_install_test_dir(
    SRC testenv/testUsdUtilsDependencyExtractor
    DEST testUsdUtilsDependencyExtractor4
)

pxr_install_test_dir(
    SRC testenv/testUsdUtilsDependencyExtractor
    DEST testUsdUtilsDependencyExtractor5
)

pxr_install_test_dir(
    SRC testenv/testUsdUtilsFlattenLayerStack
    DEST testUsdUtilsFlattenLayerStack
)

pxr_install_test_dir(
    SRC testenv/testUsdUtilsIntrospection
    DEST testUsdUtilsIntrospection
)

pxr_install_test_dir(
    SRC testenv/testUsdUtilsLocalizeAsset
    DEST testUsdUtilsLocalizeAsset
)

pxr_install_test_dir(
    SRC testenv/testUsdUtilsLocalizeAsset
    DEST testUsdUtilsLocalizeAssetUdims
)

pxr_install_test_dir(
    SRC testenv/testUsdUtilsModifyAssetPaths
    DEST testUsdUtilsModifyAssetPaths
)

pxr_install_test_dir(
    SRC testenv/testUsdUtilsPipeline
    DEST testUsdUtilsPipeline
)

pxr_install_test_dir(
    SRC testenv/testUsdUtilsStitch.testenv
    DEST testUsdUtilsStitch
)

pxr_install_test_dir(
    SRC testenv/testUsdUtilsStitchClips.testenv
    DEST testUsdUtilsStitchClips
)

pxr_install_test_dir(
    SRC testenv/testUsdUtilsUpdateSchemaWithSdrNode
    DEST testUsdUtilsUpdateSchemaWithSdrNode
)

pxr_install_test_dir(
    SRC testenv/testUsdUtilsUpdateSchemaWithSdrNode
    DEST testUsdUtilsUpdateSchemaWithSdrNodeError
)

pxr_install_test_dir(
    SRC testenv/testUsdUtilsUsdzUtils
    DEST testUsdUtilsUsdzUtils
)

pxr_install_test_dir(
    SRC testenv/testUsdUtilsUserProcessingFunc
    DEST testUsdUtilsUserProcessingFunc
)

pxr_register_test(testUsdUtilsAuthoring
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdUtilsAuthoring"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdUtilsCoalescingDiagnosticDelegate
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdUtilsCoalescingDiagnosticDelegate"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdUtilsConditionalAbortDiagnosticDelegateCase1
    PYTHON
    COMMAND
    "${CMAKE_INSTALL_PREFIX}/tests/testUsdUtilsConditionalAbortDiagnosticDelegate case1"
    EXPECTED_RETURN_CODE 134
)

pxr_register_test(testUsdUtilsConditionalAbortDiagnosticDelegateCase2
    PYTHON
    COMMAND
    "${CMAKE_INSTALL_PREFIX}/tests/testUsdUtilsConditionalAbortDiagnosticDelegate case2"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdUtilsConditionalAbortDiagnosticDelegateCase3
    PYTHON
    COMMAND
    "${CMAKE_INSTALL_PREFIX}/tests/testUsdUtilsConditionalAbortDiagnosticDelegate case3"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdUtilsConditionalAbortDiagnosticDelegateCase4
    PYTHON
    COMMAND
    "${CMAKE_INSTALL_PREFIX}/tests/testUsdUtilsConditionalAbortDiagnosticDelegate case4"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdUtilsConditionalAbortDiagnosticDelegateCase5
    PYTHON
    COMMAND
    "${CMAKE_INSTALL_PREFIX}/tests/testUsdUtilsConditionalAbortDiagnosticDelegate case5"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdUtilsConditionalAbortDiagnosticDelegateCase5a
    PYTHON
    COMMAND
    "${CMAKE_INSTALL_PREFIX}/tests/testUsdUtilsConditionalAbortDiagnosticDelegate case5a"
    EXPECTED_RETURN_CODE 134
)

pxr_register_test(testUsdUtilsConditionalAbortDiagnosticDelegateCase6
    PYTHON
    COMMAND
    "${CMAKE_INSTALL_PREFIX}/tests/testUsdUtilsConditionalAbortDiagnosticDelegate case6"
    EXPECTED_RETURN_CODE 134
)

pxr_register_test(testUsdUtilsConditionalAbortDiagnosticDelegateCase6a
    PYTHON
    COMMAND
    "${CMAKE_INSTALL_PREFIX}/tests/testUsdUtilsConditionalAbortDiagnosticDelegate case6a"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdUtilsConditionalAbortDiagnosticDelegateCase7
    PYTHON
    COMMAND
    "${CMAKE_INSTALL_PREFIX}/tests/testUsdUtilsConditionalAbortDiagnosticDelegate case7"
    EXPECTED_RETURN_CODE 134
)

pxr_register_test(testUsdUtilsConditionalAbortDiagnosticDelegateCase8
    PYTHON
    COMMAND
    "${CMAKE_INSTALL_PREFIX}/tests/testUsdUtilsConditionalAbortDiagnosticDelegate case8"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdUtilsConditionalAbortDiagnosticDelegateCase9
    PYTHON
    COMMAND
    "${CMAKE_INSTALL_PREFIX}/tests/testUsdUtilsConditionalAbortDiagnosticDelegate case9"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdUtilsConditionalAbortDiagnosticDelegateCase10
    PYTHON
    COMMAND
    "${CMAKE_INSTALL_PREFIX}/tests/testUsdUtilsConditionalAbortDiagnosticDelegate case10"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdUtilsConditionalAbortDiagnosticDelegateCase10a
    PYTHON
    COMMAND
    "${CMAKE_INSTALL_PREFIX}/tests/testUsdUtilsConditionalAbortDiagnosticDelegate case10a"
    EXPECTED_RETURN_CODE 134
)

pxr_register_test(testUsdUtilsConditionalAbortDiagnosticDelegateCase11
    PYTHON
    COMMAND
    "${CMAKE_INSTALL_PREFIX}/tests/testUsdUtilsConditionalAbortDiagnosticDelegate case11"
    EXPECTED_RETURN_CODE 134
)

pxr_register_test(testUsdUtilsConditionalAbortDiagnosticDelegateCase11a
    PYTHON
    COMMAND
    "${CMAKE_INSTALL_PREFIX}/tests/testUsdUtilsConditionalAbortDiagnosticDelegate case11a"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdUtilsConditionalAbortDiagnosticDelegateCase12
    PYTHON
    COMMAND
    "${CMAKE_INSTALL_PREFIX}/tests/testUsdUtilsConditionalAbortDiagnosticDelegate case12"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdUtilsConditionalAbortDiagnosticDelegateCase12a
    PYTHON
    COMMAND
    "${CMAKE_INSTALL_PREFIX}/tests/testUsdUtilsConditionalAbortDiagnosticDelegate case12a"
    EXPECTED_RETURN_CODE 134
)

pxr_register_test(testUsdUtilsConditionalAbortDiagnosticDelegateCase13
    PYTHON
    COMMAND
    "${CMAKE_INSTALL_PREFIX}/tests/testUsdUtilsConditionalAbortDiagnosticDelegate case13"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdUtilsConditionalAbortDiagnosticDelegateCase13a
    PYTHON
    COMMAND
    "${CMAKE_INSTALL_PREFIX}/tests/testUsdUtilsConditionalAbortDiagnosticDelegate case13a"
    EXPECTED_RETURN_CODE 134
)

pxr_register_test(testUsdUtilsConditionalAbortDiagnosticDelegateCase14
    PYTHON
    COMMAND
    "${CMAKE_INSTALL_PREFIX}/tests/testUsdUtilsConditionalAbortDiagnosticDelegate case14"
    EXPECTED_RETURN_CODE 134
)

pxr_register_test(testUsdUtilsConditionalAbortDiagnosticDelegateCase14a
    PYTHON
    COMMAND
    "${CMAKE_INSTALL_PREFIX}/tests/testUsdUtilsConditionalAbortDiagnosticDelegate case14a"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdUtilsConditionalAbortDiagnosticDelegateCase15
    PYTHON
    COMMAND
    "${CMAKE_INSTALL_PREFIX}/tests/testUsdUtilsConditionalAbortDiagnosticDelegate case15"
    EXPECTED_RETURN_CODE 134
)

pxr_register_test(testUsdUtilsConditionalAbortDiagnosticDelegateCase16
    PYTHON
    COMMAND
    "${CMAKE_INSTALL_PREFIX}/tests/testUsdUtilsConditionalAbortDiagnosticDelegate case16"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdUtilsCoalescingDiagnosticDelegateCpp
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdUtilsCoalescingDiagnosticDelegateCpp"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdUtilsCreateNewARKitUsdzPackage
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdUtilsCreateNewUsdzPackage --check --arkit -l contents_arkit.txt package.usdz root.usd"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdUtilsCreateNewARKitUsdzPackageInPlace
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdUtilsCreateNewUsdzPackage --editLayersInPlace --expectedDirtyLayers flat.usda --check --arkit package_flat.usdz flat.usda -l contents_arkit_flat_in_place.txt"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdUtilsCreateNewUsdzPackage
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdUtilsCreateNewUsdzPackage --check -l contents.txt package.usdz root.usd"
    DIFF_COMPARE contents.txt
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdUtilsCreateNewUsdzPackageContextDep
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdUtilsCreateNewUsdzPackage --check -l contents_usdz_context_dep_assets.txt usdzContextDepAssets.usdz usdzContextDepAssets/usdzContextDepAssets.usda"
    DIFF_COMPARE contents_usdz_context_dep_assets.txt
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdUtilsCreateNewUsdzPackageFilterMetadataKeys
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdUtilsCreateNewUsdzPackage --check -l contents_filter_metadata_keys.txt filter_metadata_keys.usdz filterMetadataKeys/filter_metadata_keys.usda"
    DIFF_COMPARE contents_filter_metadata_keys.txt
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdUtilsCreateNewUsdzPackageUdim
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdUtilsCreateNewUsdzPackage --check -l contents_udim.txt udim.usdz udim/root.usda"
    DIFF_COMPARE contents_udim.txt
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdUtilsCreateNewUsdzPackageRemapClipsDir
   PYTHON
   COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdUtilsCreateNewUsdzPackage --check remapClipsDir.usdz remapClipsDir/root.usda -l contents_remap_clips_dir.txt"
   DIFF_COMPARE contents_remap_clips_dir.txt
   EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdUtilsCreateNewUsdzPackageEditInPlace
   PYTHON
   COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdUtilsCreateNewUsdzPackage --editLayersInPlace --expectedDirtyLayers nestedUsdz/combined.usda --check combined.usdz nestedUsdz/combined.usda -l contents_nested_in_place.txt"
   DIFF_COMPARE contents_nested_in_place.txt
   EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdUtilsDependencies
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdUtilsDependencies"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdUtilsDependenciesCustomResolver
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdUtilsDependenciesCustomResolver"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdUtilsDependencyExtractor1
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdUtilsDependencyExtractor ascii.usda ascii-usda.txt"
    DIFF_COMPARE ascii-usda.txt
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdUtilsDependencyExtractor2
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdUtilsDependencyExtractor ascii.usd ascii-usd.txt"
    DIFF_COMPARE ascii-usd.txt
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdUtilsDependencyExtractor3
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdUtilsDependencyExtractor crate.usdc crate-usdc.txt"
    DIFF_COMPARE crate-usdc.txt
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdUtilsDependencyExtractor4
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdUtilsDependencyExtractor crate.usd crate-usd.txt"
    DIFF_COMPARE crate-usd.txt
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdUtilsDependencyExtractor5
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdUtilsDependencyExtractor --open-as-anon anon.usda anon.txt"
    DIFF_COMPARE anon.txt
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdUtilsFlattenLayerStack
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdUtilsFlattenLayerStack"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdUtilsIntrospection
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdUtilsIntrospection"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdUtilsLocalizeAsset
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdUtilsLocalizeAsset default/root.usda default_localized -l contents_default.txt"
    DIFF_COMPARE contents_default.txt
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdUtilsLocalizeAssetUdims
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdUtilsLocalizeAsset udim/root.usda udim_localized -l contents_udim.txt"
    DIFF_COMPARE contents_udim.txt
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdUtilsModifyAssetPaths
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdUtilsModifyAssetPaths layer.usda modified.usda"
    DIFF_COMPARE modified.usda duplicates.usda removal.usda removal_preserve_length.usda ref_layer_mod.usda ref_unmodified.usda
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdUtilsPipeline
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdUtilsPipeline"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdUtilsSparseValueWriter
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdUtilsSparseValueWriter"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdUtilsStageCache
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdUtilsStageCache"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdUtilsStitch
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdUtilsStitch"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdUtilsStitchClips
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdUtilsStitchClips"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdUtilsUpdateSchemaWithSdrNode
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdUtilsUpdateSchemaWithSdrNode"
    DIFF_COMPARE resultAPISchema.usda
    DIFF_COMPARE resultAPIIdentifierMissing.usda
    DIFF_COMPARE result_override.usda
    DIFF_COMPARE resultUsdShadeConnectableAPIMetadata.usda
    DIFF_COMPARE resultUsdShadeConnectableAPIMetadata2.usda
    DIFF_COMPARE duplicateProp.usda
    DIFF_COMPARE duplicatePropTypeMisMatch.usda
    DIFF_COMPARE schemaConcrete.usda
    DIFF_COMPARE testSdrNodeNonShaderNamespacePrefixResult.usda
    DIFF_COMPARE testSdrNodeShaderNamespacePrefixResult.usda
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdUtilsUpdateSchemaWithSdrNodeError
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdUtilsUpdateSchemaWithSdrNode TestUsdUpdateSchemaWithSdrNode True"
    EXPECTED_RETURN_CODE 1
)

pxr_register_test(testUsdUtilsStitchCpp
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdUtilsStitchCpp"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdUtilsTimeCodeRange
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdUtilsTimeCodeRange"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdUtilsTimeCodeRangeCpp
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdUtilsTimeCodeRangeCpp"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdUtilsUserProcessingFunc
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdUtilsUserProcessingFunc"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdUtilsVarSelsSessionLayer
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdUtilsVarSelsSessionLayer"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdUtilsUsdzUtils
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdUtilsUsdzUtils"
    EXPECTED_RETURN_CODE 0
)
