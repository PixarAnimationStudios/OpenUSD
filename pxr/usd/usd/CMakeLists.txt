set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usd)

pxr_library(usd
    LIBRARIES
        arch
        kind
        pcp
        sdf
        ar
        plug
        tf
        trace
        vt
        work
        ${Boost_PYTHON_LIBRARY}
        ${TBB_tbb_LIBRARY}

    INCLUDE_DIRS
        ${Boost_INCLUDE_DIRS}
        ${TBB_INCLUDE_DIRS}

    PUBLIC_CLASSES
        apiSchemaBase
        attribute
        clipsAPI
        attributeQuery
        collectionAPI
        collectionMembershipQuery
        common
        crateInfo
        debugCodes
        editContext
        editTarget
        flattenUtils
        inherits
        interpolation
        modelAPI
        notice
        object
        payloads
        prim
        primCompositionQuery
        primData
        primDataHandle
        primDefinition
        primFlags
        primRange
        primTypeInfo
        property
        references
        relationship
        resolveInfo
        resolver
        schemaBase
        schemaRegistry
        specializes
        stage
        stageCache
        stageCacheContext
        stageLoadRules
        stagePopulationMask
        timeCode
        tokens
        typed
        usdFileFormat
        usdaFileFormat
        usdcFileFormat
        usdzFileFormat
        variantSets
        zipFile

    PYTHON_PUBLIC_CLASSES
        pyEditContext
        pyConversions

    PRIVATE_CLASSES
        clip
        clipCache
        clipSet
        clipSetDefinition
        crateData
        crateFile
        instanceCache
        instanceKey
        integerCoding
        interpolators
        primTypeInfoCache
        usdzResolver
        valueUtils

    PUBLIC_HEADERS
        api.h

    PRIVATE_HEADERS
        crateDataTypes.h
        crateValueInliners.h
        listEditImpl.h
        wrapUtils.h

    PYTHON_CPPFILES
        moduleDeps.cpp

    PYMODULE_CPPFILES
        module.cpp
        wrapAPISchemaBase.cpp
        wrapAttribute.cpp
        wrapAttributeQuery.cpp
        wrapClipsAPI.cpp
        wrapCollectionAPI.cpp
        wrapCollectionMembershipQuery.cpp
        wrapCommon.cpp
        wrapCrateInfo.cpp
        wrapEditContext.cpp
        wrapEditTarget.cpp
        wrapFlattenUtils.cpp
        wrapInherits.cpp
        wrapInterpolation.cpp
        wrapModelAPI.cpp
        wrapNotice.cpp
        wrapObject.cpp
        wrapPayloads.cpp
        wrapPrim.cpp
        wrapPrimCompositionQuery.cpp
        wrapPrimDefinition.cpp
        wrapPrimFlags.cpp
        wrapPrimRange.cpp
        wrapPrimTypeInfo.cpp
        wrapProperty.cpp
        wrapReferences.cpp
        wrapRelationship.cpp
        wrapResolveInfo.cpp
        wrapSchemaBase.cpp
        wrapSchemaRegistry.cpp
        wrapSpecializes.cpp
        wrapStage.cpp
        wrapStageCache.cpp
        wrapStageCacheContext.cpp
        wrapStageLoadRules.cpp
        wrapStagePopulationMask.cpp
        wrapTokens.cpp
        wrapTimeCode.cpp
        wrapTyped.cpp
        wrapUtils.cpp
        wrapVariantSets.cpp
        wrapVersion.cpp 
        wrapZipFile.cpp

    PYMODULE_FILES
        __init__.py

    RESOURCE_FILES
        plugInfo.json
        generatedSchema.usda
        schema.usda:usd/schema.usda
        codegenTemplates/api.h
        codegenTemplates/plugInfo.json:no_subst
        codegenTemplates/schemaClass.cpp
        codegenTemplates/schemaClass.h
        codegenTemplates/tokens.cpp
        codegenTemplates/tokens.h
        codegenTemplates/wrapSchemaClass.cpp
        codegenTemplates/wrapTokens.cpp

    DOXYGEN_FILES
        doxygen/advancedFeatures.dox
        doxygen/authoringEditing.dox
        doxygen/bestPractices.dox
        doxygen/commonIdioms.dox
        doxygen/datatypes.dox
        doxygen/dynamicFileFormat.dox
        doxygen/front.dox
        doxygen/generatingSchemas.dox
        doxygen/instancing.dox
        doxygen/multiThreading.dox
        doxygen/objectModel.dox
        doxygen/propertiesOfSceneDescription.dox 
        doxygen/valueClips.dox
        doxygen/images/instancing/Instancing_Example.png
        doxygen/images/instancing/Nested_Instancing_Example.png
        doxygen/images/instancing/Relationship_Example.png
        doxygen/images/instancing/Uninstanced_vs_Instanced.png
)

if (NOT PXR_ENABLE_PYTHON_SUPPORT)
    message(STATUS "Skipping building usdGenSchema, Python modules required.")
elseif (NOT JINJA2_FOUND)
    message(WARNING "Skipping building usdGenSchema due to missing dependency: Jinja2")
else()
    pxr_python_bin(usdGenSchema
        DEPENDENCIES
            tf
            sdf
            usd
    )
endif()

pxr_test_scripts(
    testenv/testUsdAppliedAPISchemas.py
    testenv/testUsdAttributeBlocking.py
    testenv/testUsdAttributeConnections.py
    testenv/testUsdBugs.py
    testenv/testUsdBug119633.py
    testenv/testUsdBug141491.py
    testenv/testUsdChangeProcessing.py
    testenv/testUsdClasses.py
    testenv/testUsdCollectionAPI.py
    testenv/testUsdCratePayloadConversionFromVersion07.py
    testenv/testUsdCreateProperties.py
    testenv/testUsdDataFormats.py
    testenv/testUsdExternalAssetDependencies.py
    testenv/testUsdFallbackPrimTypes.py
    testenv/testUsdFileFormats.py
    testenv/testUsdFlatten.py
    testenv/testUsdFlatten2.py
    testenv/testUsdFlattenLayerStack.py
    testenv/testUsdFlattenProperties.py
    testenv/testUsdInherits.py
    testenv/testUsdInstanceProxy.py
    testenv/testUsdInstancing.py
    testenv/testUsdLoadUnloadDeepNestedInstancing.py
    testenv/testUsdMetadata.py
    testenv/testUsdModel.py
    testenv/testUsdNotices.py
    testenv/testUsdPayloads.py
    testenv/testUsdPrimCompositionQuery.py
    testenv/testUsdPrimFlagsPredicate.py
    testenv/testUsdPrimRange.py
    testenv/testUsdPrims.py
    testenv/testUsdReferences.py
    testenv/testUsdRelationships.py
    testenv/testUsdSchemaBasePy.py
    testenv/testUsdSchemaRegistry.py
    testenv/testUsdSpecializes.py
    testenv/testUsdStage.py
    testenv/testUsdStageCache.py
    testenv/testUsdStageLoadUnload.py
    testenv/testUsdStagePopulationMasks.py
    testenv/testUsdTimeCodeRepr.py
    testenv/testUsdTimeOffsets.py
    testenv/testUsdTimeSamples.py
    testenv/testUsdTimeValueAuthoring.py
    testenv/testUsdUsdzFileFormat.py
    testenv/testUsdValueClips.py
    testenv/testUsdVariantEditing.py
    testenv/testUsdVariantFallbacks.py
    testenv/testUsdVariants.py
    testenv/testUsdZipFile.py
)

pxr_build_test_shared_lib(TestUsdProceduralExternalAssetsFileFormatPlugin
    CREATE_FRAMEWORK
    INSTALL_PREFIX UsdPlugins
    LIBRARIES
        sdf
        tf
        usd
        
    CPPFILES
        testenv/TestUsdProceduralExternalAssetsFileFormatPlugin.cpp
)

pxr_build_test(testUsdAttributeBlockingCpp
    LIBRARIES
        tf
        usd
    CPPFILES
        testenv/testUsdAttributeBlocking.cpp
)

pxr_build_test(testUsdAttributeInterpolationCpp
    LIBRARIES
        usd
    CPPFILES
        testenv/testUsdAttributeInterpolationCpp.cpp
)

pxr_build_test(testUsdCreateAttributeCpp
    LIBRARIES
        usd
    CPPFILES
        testenv/testUsdCreateAttributeCpp.cpp
)

pxr_build_test(testUsdHardToReach
    LIBRARIES
        usd
    CPPFILES
        testenv/testUsdHardToReach.cpp
)

pxr_build_test(testUsdIntegerCoding
    LIBRARIES
        usd
    CPPFILES
        testenv/testUsdIntegerCoding.cpp
)

pxr_build_test(testUsdMetadataCpp
    LIBRARIES
        usd
    CPPFILES
        testenv/testUsdMetadata.cpp
)

pxr_build_test(testUsdInstancingCpp
    LIBRARIES
        tf
        usd
        sdf
    CPPFILES
        testenv/testUsdInstancingCpp.cpp
)

pxr_build_test(testUsdPrimGetDescendants
    LIBRARIES
        tf
        usd
    CPPFILES
        testenv/testUsdPrimGetDescendants.cpp
)

pxr_build_test(testUsdStageNotification
    LIBRARIES
        usd
    CPPFILES
        testenv/testUsdStageNotification.cpp
)

pxr_build_test(testUsdStageNoPython
    LIBRARIES
        usd
    CPPFILES
        testenv/testUsdStageNoPython.cpp
)

pxr_build_test(testUsdTemplatedIO
    LIBRARIES
        usd
    CPPFILES
        testenv/testUsdTemplatedIO.cpp
)

pxr_build_test(testUsdStageThreading
    LIBRARIES
        arch
        tf
        work
        sdf
        usd
        ${Boost_PROGRAM_OPTIONS_LIBRARY}
    CPPFILES
        testenv/testUsdStageThreading.cpp
)

pxr_build_test(testUsdThreadedAuthoring
    LIBRARIES
        arch
        tf
        work
        sdf
        usd
    CPPFILES
        testenv/testUsdThreadedAuthoring.cpp
)

pxr_build_test(testUsdTimeCodeStream
    LIBRARIES
        tf
        usd
    CPPFILES
        testenv/testUsdTimeCodeStream.cpp
)

pxr_build_test(testUsdSchemaBase
    LIBRARIES
        usd
    CPPFILES
        testenv/testUsdSchemaBase.cpp
)

pxr_build_test(testUsdTimeValueAuthoringCpp
    LIBRARIES
        arch
        plug
        sdf
        tf
        usd
    CPPFILES
        testenv/testUsdTimeValueAuthoring.cpp
)

pxr_build_test(testUsdUsdzResolver
    LIBRARIES
        ar
        arch
        tf
        usd
    CPPFILES
        testenv/testUsdUsdzResolver.cpp
)

pxr_install_test_dir(
    SRC testenv/testUsdAppliedAPISchemas
    DEST testUsdAppliedAPISchemas
)

pxr_install_test_dir(
    SRC testenv/testUsdStageThreading
    DEST testUsdStageThreading
)

pxr_install_test_dir(
    SRC testenv/testUsdCratePayloadConversionFromVersion07.testenv
    DEST testUsdCratePayloadConversionFromVersion07
)

pxr_install_test_dir(
    SRC testenv/testUsdExternalAssetDependencies.testenv
    DEST testUsdExternalAssetDependencies
)

pxr_install_test_dir(
    SRC testenv/testUsdFallbackPrimTypes
    DEST testUsdFallbackPrimTypes
)

pxr_install_test_dir(
    SRC testenv/testUsdFlatten
    DEST testUsdFlatten
)

pxr_install_test_dir(
    SRC testenv/testUsdFlatten2
    DEST testUsdFlatten2
)

pxr_install_test_dir(
    SRC testenv/testUsdFlatten2
    DEST testUsdFlatten3
)

pxr_install_test_dir(
    SRC testenv/testUsdFlatten2
    DEST testUsdFlatten4
)

pxr_install_test_dir(
    SRC testenv/testUsdFlattenLayerStack
    DEST testUsdFlattenLayerStack
)

pxr_install_test_dir(
    SRC testenv/testUsdFlattenProperties
    DEST testUsdFlattenProperties
)

pxr_install_test_dir(
    SRC testenv/testUsdFileFormats
    DEST testUsdFileFormats
)

pxr_install_test_dir(
    SRC testenv/testUsdFileFormats
    DEST testUsdFileFormats_pread
)

pxr_install_test_dir(
    SRC testenv/testUsdVariantEditing.testenv
    DEST testUsdEditVariantEditing
)

pxr_install_test_dir(
    SRC testenv/testUsdInstancing
    DEST testUsdInstancing
)

pxr_install_test_dir(
    SRC testenv/testUsdInstanceProxy
    DEST testUsdInstanceProxy
)

pxr_install_test_dir(
    SRC testenv/testUsdMetadata
    DEST testUsdMetadata
)

pxr_install_test_dir(
    SRC testenv/testUsdNotices.testenv
    DEST testUsdNotices
)

pxr_install_test_dir(
    SRC testenv/testUsdPrims.testenv
    DEST testUsdPrims
)

pxr_install_test_dir(
    SRC testenv/testUsdPrimCompositionQuery.testenv
    DEST testUsdPrimCompositionQuery
)

pxr_install_test_dir(
    SRC testenv/testUsdPrimGetDescendants.testenv
    DEST testUsdPrimGetDescendants
)

pxr_install_test_dir(
    SRC testenv/testUsdInstancingCpp.testenv
    DEST testUsdInstancingCpp
)

pxr_install_test_dir(
    SRC testenv/testUsdVariantEditing.testenv
    DEST testUsdVariantEditing
)

pxr_install_test_dir(
    SRC testenv/testUsdVariantFallbacks.testenv
    DEST testUsdVariantFallbacks
)

pxr_install_test_dir(
    SRC testenv/testUsdSchemaGen
    DEST testUsdSchemaGen
)

pxr_install_test_dir(
    SRC testenv/testUsdSchemaRegistry
    DEST testUsdSchemaRegistry
)

pxr_install_test_dir(
    SRC testenv/testUsdTimeValueAuthoring
    DEST testUsdTimeValueAuthoring
)

pxr_install_test_dir(
    SRC testenv/testUsdTimeValueAuthoring
    DEST testUsdTimeValueAuthoringCpp
)

pxr_install_test_dir(
    SRC testenv/testUsdBug119633.testenv
    DEST testUsdBug119633
)

pxr_install_test_dir(
    SRC testenv/testUsdBug141491.testenv
    DEST testUsdBug141491
)

pxr_install_test_dir(
    SRC testenv/testUsdLoadUnloadDeepNestedInstancing.testenv
    DEST testUsdLoadUnloadDeepNestedInstancing
)

pxr_install_test_dir(
    SRC testenv/testUsdStageNoPython.testenv
    DEST testUsdStageNoPython
)

pxr_install_test_dir(
    SRC testenv/testUsdPrimRange
    DEST testUsdPrimRange
)

pxr_install_test_dir(
    SRC testenv/testUsdValueClips
    DEST testUsdValueClips
)

pxr_install_test_dir(
    SRC testenv/testUsdVariants
    DEST testUsdVariants
)

pxr_install_test_dir(
    SRC testenv/testUsdCollectionAPI
    DEST testUsdCollectionAPI
)

pxr_install_test_dir(
    SRC testenv/testUsdUsdzFileFormat
    DEST testUsdUsdzFileFormat
)

pxr_install_test_dir(
    SRC testenv/testUsdUsdzFileFormat
    DEST testUsdUsdzFileFormat_pread
)

pxr_install_test_dir(
    SRC testenv/testUsdUsdzResolver
    DEST testUsdUsdzResolver
)

pxr_install_test_dir(
    SRC testenv/testUsdZipFile.testenv
    DEST testUsdZipFile
)

pxr_register_test(testUsdAppliedAPISchemas
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdAppliedAPISchemas"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdAttributeBlocking
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdAttributeBlocking"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdAttributeBlockingCpp
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdAttributeBlockingCpp"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdAttributeConnections
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdAttributeConnections"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdValueClips
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdValueClips"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdVariantEditing
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdVariantEditing"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdVariantFallbacks
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdVariantFallbacks"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdVariants
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdVariants"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdAttributeInterpolationCpp
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdAttributeInterpolationCpp"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdBug119633
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdBug119633"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdBug141491
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdBug141491"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdChangeProcessing
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdChangeProcessing"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdCollectionAPI
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdCollectionAPI"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdClasses
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdClasses"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdCreateProperties
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdCreateProperties"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdCreateAttributeCpp
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdCreateAttributeCpp"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdExternalAssetDependencies
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdExternalAssetDependencies"
    EXPECTED_RETURN_CODE 0  
)

pxr_register_test(testUsdFallbackPrimTypes
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdFallbackPrimTypes"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdHardToReach
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdHardToReach"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdInstancing
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdInstancing"
    ENV
        USD_ASSIGN_PROTOTYPES_DETERMINISTICALLY=1
)

pxr_register_test(testUsdInstanceProxy
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdInstanceProxy"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdIntegerCoding
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdIntegerCoding"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdDataFormats
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdDataFormats"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdLoadUnloadDeepNestedInstancing
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdLoadUnloadDeepNestedInstancing"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdTimeSamples
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdTimeSamples"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdReferences
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdReferences"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdTemplatedIO
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdTemplatedIO"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdPrimCompositionQuery
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdPrimCompositionQuery"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdPrimGetDescendants
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdPrimGetDescendants"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdInstancingCpp
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdInstancingCpp"
    EXPECTED_RETURN_CODE 0
    ENV
        USD_ASSIGN_PROTOTYPES_DETERMINISTICALLY=1
)

pxr_register_test(testUsdPrims
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdPrims"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdFileFormats
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdFileFormats"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdFileFormats_pread
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdFileFormats"
    ENV
        USDC_USE_PREAD=1
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdMetadataCpp
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdMetadataCpp"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdStageNoPython
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdStageNoPython"
    EXPECTED_RETURN_CODE 0
)
pxr_register_test(testUsdStageNotification
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdStageNotification"
    EXPECTED_RETURN_CODE 0
)
pxr_register_test(testUsdStageThreading
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdStageThreading"
    EXPECTED_RETURN_CODE 0
)
pxr_register_test(testUsdThreadedAuthoring
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdThreadedAuthoring"
    EXPECTED_RETURN_CODE 0
)
pxr_register_test(testUsdTimeCodeStream
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdTimeCodeStream"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdStage
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdStage"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdStagePopulationMasks
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdStagePopulationMasks"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdTimeCodeRepr
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdTimeCodeRepr"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdTimeOffsets
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdTimeOffsets"
    EXPECTED_RETURN_CODE 0
    ENV
        PCP_DISABLE_TIME_SCALING_BY_LAYER_TCPS=0
)

pxr_register_test(testUsdPrimRange
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdPrimRange"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdStageLoadUnload
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdStageLoadUnload"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdPayloads
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdPayloads"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdCratePayloadConversionFromVersion07
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdCratePayloadConversionFromVersion07"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdInherits
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdInherits"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdModel
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdModel"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdSpecializes
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdSpecializes"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdMetadata
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdMetadata"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdFlatten
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdFlatten"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdFlatten2
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdFlatten2 BasicInstancingAndVariants/root.usda"
    STDOUT_REDIRECT flat_BasicInstancingAndVariants.usda
    DIFF_COMPARE flat_BasicInstancingAndVariants.usda
    EXPECTED_RETURN_CODE 0
    ENV
        USD_ASSIGN_PROTOTYPES_DETERMINISTICALLY=1
)

pxr_register_test(testUsdFlatten3
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdFlatten2 BasicInstancing/root.usda"
    STDOUT_REDIRECT flat_BasicInstancing.usda
    DIFF_COMPARE flat_BasicInstancing.usda
    EXPECTED_RETURN_CODE 0
    ENV
        USD_ASSIGN_PROTOTYPES_DETERMINISTICALLY=1
)

pxr_register_test(testUsdFlatten4
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdFlatten2 BasicInstancingAndNestedInstances/root.usda"
    STDOUT_REDIRECT flat_BasicInstancingAndNestedInstances.usda
    DIFF_COMPARE flat_BasicInstancingAndNestedInstances.usda
    EXPECTED_RETURN_CODE 0
    ENV
        USD_ASSIGN_PROTOTYPES_DETERMINISTICALLY=1
)

pxr_register_test(testUsdFlattenLayerStack
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdFlattenLayerStack"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdNotices
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdNotices"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdRelationships
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdRelationships"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdSchemaBase
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdSchemaBase"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdSchemaBasePy
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdSchemaBasePy"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdSchemaRegistry
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdSchemaRegistry"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdTimeValueAuthoring
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdTimeValueAuthoring"
    EXPECTED_RETURN_CODE 0
    ENV
        PCP_DISABLE_TIME_SCALING_BY_LAYER_TCPS=0
)

pxr_register_test(testUsdTimeValueAuthoringCpp
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdTimeValueAuthoringCpp"
    EXPECTED_RETURN_CODE 0
    ENV
        PCP_DISABLE_TIME_SCALING_BY_LAYER_TCPS=0
)

pxr_register_test(testUsdUsdzFileFormat
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdUsdzFileFormat"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdUsdzFileFormat_pread
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdUsdzFileFormat"
    ENV
        USDC_USE_PREAD=1
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdZipFile
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdZipFile"
    EXPECTED_RETURN_CODE 0
)
pxr_register_test(testUsdUsdzResolver
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdUsdzResolver"
    EXPECTED_RETURN_CODE 0
)
