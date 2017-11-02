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
        tracelite
        vt
        work
        ${Boost_PYTHON_LIBRARY}
        ${TBB_tbb_LIBRARY}

    INCLUDE_DIRS
        ${Boost_INCLUDE_DIRS}
        ${TBB_INCLUDE_DIRS}

    PUBLIC_CLASSES
        attribute
        clipsAPI
        attributeQuery
        common
        crateInfo
        debugCodes
        editContext
        editTarget
        inherits
        interpolation
        modelAPI
        notice
        object
        prim
        primData
        primDataHandle
        primFlags
        primRange
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
        stagePopulationMask
        timeCode
        tokens
        typed
        usdFileFormat
        usdaFileFormat
        usdcFileFormat
        variantSets

    PYTHON_PUBLIC_CLASSES
        pyEditContext
        pyConversions

    PRIVATE_CLASSES
        clip
        clipCache
        crateData
        crateFile
        instanceCache
        instanceKey
        integerCoding
        interpolators

    PUBLIC_HEADERS
        api.h

    PRIVATE_HEADERS
        crateDataTypes.h
        crateValueInliners.h
        valueUtils.h
        wrapUtils.h

    PYTHON_CPPFILES
        moduleDeps.cpp

    PYMODULE_CPPFILES
        module.cpp
        wrapAttribute.cpp
        wrapAttributeQuery.cpp
        wrapClipsAPI.cpp
        wrapCommon.cpp
        wrapCrateInfo.cpp
        wrapEditContext.cpp
        wrapEditTarget.cpp
        wrapInherits.cpp
        wrapInterpolation.cpp
        wrapModelAPI.cpp
        wrapNotice.cpp
        wrapObject.cpp
        wrapPrim.cpp
        wrapPrimFlags.cpp
        wrapPrimRange.cpp
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
        wrapStagePopulationMask.cpp
        wrapTokens.cpp
        wrapTimeCode.cpp
        wrapTyped.cpp
        wrapUtils.cpp
        wrapVariantSets.cpp
        wrapVersion.cpp 

    PYMODULE_FILES
        __init__.py

    RESOURCE_FILES
        plugInfo.json
        generatedSchema.usda
        schema.usda:usd/schema.usda
        codegenTemplates/api.h
        codegenTemplates/plugInfo.json
        codegenTemplates/schemaClass.cpp
        codegenTemplates/schemaClass.h
        codegenTemplates/tokens.cpp
        codegenTemplates/tokens.h
        codegenTemplates/wrapSchemaClass.cpp
        codegenTemplates/wrapTokens.cpp
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
    testenv/testUsdAttributeBlocking.py
    testenv/testUsdAttributeConnections.py
    testenv/testUsdBug119633.py
    testenv/testUsdBug141491.py
    testenv/testUsdChangeProcessing.py
    testenv/testUsdClasses.py
    testenv/testUsdCreateProperties.py
    testenv/testUsdDataFormats.py
    testenv/testUsdFileFormats.py
    testenv/testUsdFlatten.py
    testenv/testUsdFlatten2.py
    testenv/testUsdInherits.py
    testenv/testUsdInstanceProxy.py
    testenv/testUsdInstancing.py
    testenv/testUsdMetadata.py
    testenv/testUsdModel.py
    testenv/testUsdNotices.py
    testenv/testUsdPayloads.py
    testenv/testUsdPrimFlagsPredicate.py
    testenv/testUsdPrimRange.py
    testenv/testUsdPrims.py
    testenv/testUsdReferences.py
    testenv/testUsdRelationships.py
    testenv/testUsdSchemaRegistry.py
    testenv/testUsdSpecializes.py
    testenv/testUsdStage.py
    testenv/testUsdStageCache.py
    testenv/testUsdStageLoadUnload.py
    testenv/testUsdStagePopulationMasks.py
    testenv/testUsdTimeOffsets.py
    testenv/testUsdTimeSamples.py
    testenv/testUsdValueClips.py
    testenv/testUsdVariantEditing.py
    testenv/testUsdVariantFallbacks.py
    testenv/testUsdVariants.py
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

pxr_build_test(testUsdSchemaBase
    LIBRARIES
        usd
    CPPFILES
        testenv/testUsdSchemaBase.cpp
)

pxr_install_test_dir(
    SRC testenv/testUsdStageThreading
    DEST testUsdStageThreading
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
    SRC testenv/testUsdFileFormats
    DEST testUsdFileFormats
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
    SRC testenv/testUsdMetadata
    DEST testUsdMetadata_OldOffsets
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
    SRC testenv/testUsdPrimGetDescendants.testenv
    DEST testUsdPrimGetDescendants
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
    SRC testenv/testUsdBug119633.testenv
    DEST testUsdBug119633
)

pxr_install_test_dir(
    SRC testenv/testUsdBug141491.testenv
    DEST testUsdBug141491
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
    SRC testenv/testUsdValueClips
    DEST testUsdValueClips_OldOffsets
)

pxr_install_test_dir(
    SRC testenv/testUsdValueClips_Legacy
    DEST testUsdValueClips_Legacy
)

pxr_install_test_dir(
    SRC testenv/testUsdVariants
    DEST testUsdVariants
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
    ENV
        USD_USE_INVERSE_LAYER_OFFSET=0
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdValueClips_OldOffsets
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdValueClips"
    ENV
        USD_USE_INVERSE_LAYER_OFFSET=1
    EXPECTED_RETURN_CODE 0
)

# This test runs the same test script as testUsdValueClips
# but uses a different set of test assets that are installed above.
pxr_register_test(testUsdValueClips_Legacy
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdValueClips"
    EXPECTED_RETURN_CODE 0
    ENV
        USD_AUTHOR_LEGACY_CLIPS=1
        USD_READ_LEGACY_CLIPS=1
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
    ENV
        USD_USE_INVERSE_LAYER_OFFSET=0
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdAttributeInterpolationCpp_OldOffsets
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdAttributeInterpolationCpp"
    ENV
        USD_USE_INVERSE_LAYER_OFFSET=1
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

pxr_register_test(testUsdInstancing
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdInstancing"
    ENV
        USD_ASSIGN_MASTERS_DETERMINISTICALLY=1
)

pxr_register_test(testUsdInstanceProxy
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdInstanceProxy"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdDataFormats
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdDataFormats"
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

pxr_register_test(testUsdPrimGetDescendants
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdPrimGetDescendants"
    EXPECTED_RETURN_CODE 0
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

pxr_register_test(testUsdStage
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdStage"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdTimeOffsets
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdTimeOffsets"
    ENV
        USD_USE_INVERSE_LAYER_OFFSET=0
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdTimeOffsets_OldOffsets
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdTimeOffsets"
    ENV
        USD_USE_INVERSE_LAYER_OFFSET=1
    EXPECTED_RETURN_CODE 0
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
    ENV
        USD_USE_INVERSE_LAYER_OFFSET=0
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdMetadata_OldOffsets
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdMetadata"
    ENV
        USD_USE_INVERSE_LAYER_OFFSET=1
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
)

pxr_register_test(testUsdFlatten3
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdFlatten2 BasicInstancing/root.usda"
    STDOUT_REDIRECT flat_BasicInstancing.usda
    DIFF_COMPARE flat_BasicInstancing.usda
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdFlatten4
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdFlatten2 BasicInstancingAndNestedInstances/root.usda"
    STDOUT_REDIRECT flat_BasicInstancingAndNestedInstances.usda
    DIFF_COMPARE flat_BasicInstancingAndNestedInstances.usda
    EXPECTED_RETURN_CODE 0
    ENV
        USD_ASSIGN_MASTERS_DETERMINISTICALLY=1
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

pxr_register_test(testUsdSchemaRegistry
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdSchemaRegistry"
    EXPECTED_RETURN_CODE 0
)
