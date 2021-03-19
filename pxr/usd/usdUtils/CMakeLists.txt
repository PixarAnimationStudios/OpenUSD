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
        ${Boost_PYTHON_LIBRARY}

    INCLUDE_DIRS
        ${Boost_INCLUDE_DIRS}

    PUBLIC_CLASSES
        authoring
        coalescingDiagnosticDelegate
        conditionalAbortDiagnosticDelegate
        debugCodes
        dependencies
        flattenLayerStack
        introspection
        pipeline
        registeredVariantSet
        sparseValueWriter
        stageCache
        stitch
        stitchClips
        timeCodeRange

    PUBLIC_HEADERS
        api.h

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
        wrapPipeline.cpp
        wrapRegisteredVariantSet.cpp
        wrapSparseValueWriter.cpp
        wrapStageCache.cpp
        wrapStitch.cpp
        wrapStitchClips.cpp
        wrapTimeCodeRange.cpp

    PYMODULE_FILES
        __init__.py
        complianceChecker.py
        constantsGroup.py

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
    testenv/testUsdUtilsDependencyExtractor.py
    testenv/testUsdUtilsFlattenLayerStack.py
    testenv/testUsdUtilsIntrospection.py
    testenv/testUsdUtilsModifyAssetPaths.py
    testenv/testUsdUtilsPipeline.py
    testenv/testUsdUtilsSparseValueWriter.py
    testenv/testUsdUtilsStageCache.py
    testenv/testUsdUtilsStitch.py
    testenv/testUsdUtilsStitchClips.py
    testenv/testUsdUtilsTimeCodeRange.py
    testenv/testUsdUtilsVarSelsSessionLayer.py
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
    DEST testUsdUtilsCreateNewUsdzPackage
)

pxr_install_test_dir(
    SRC testenv/testUsdUtilsDependencies
    DEST testUsdUtilsDependencies
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
    SRC testenv/testUsdUtilsFlattenLayerStack
    DEST testUsdUtilsFlattenLayerStack
)

pxr_install_test_dir(
    SRC testenv/testUsdUtilsIntrospection
    DEST testUsdUtilsIntrospection
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

pxr_register_test(testUsdUtilsCreateNewUsdzPackage
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdUtilsCreateNewUsdzPackage --check -l contents.txt package.usdz root.usd"
    DIFF_COMPARE contents.txt
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdUtilsDependencies
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdUtilsDependencies"
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

pxr_register_test(testUsdUtilsModifyAssetPaths
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdUtilsModifyAssetPaths layer.usda modified.usda"
    DIFF_COMPARE modified.usda
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

pxr_register_test(testUsdUtilsVarSelsSessionLayer
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdUtilsVarSelsSessionLayer"
    EXPECTED_RETURN_CODE 0
)
