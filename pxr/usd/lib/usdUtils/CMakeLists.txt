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
        dependencies
        flattenLayerStack
        introspection
        pipeline
        registeredVariantSet
        stageCache
        stitch
        stitchClips

    PUBLIC_HEADERS
        api.h
        
    PYTHON_CPPFILES
        moduleDeps.cpp

    PYMODULE_CPPFILES
        module.cpp
        wrapAuthoring.cpp
        wrapDependencies.cpp
        wrapFlattenLayerStack.cpp
        wrapIntrospection.cpp
        wrapPipeline.cpp
        wrapRegisteredVariantSet.cpp
        wrapStageCache.cpp
        wrapStitch.cpp
        wrapStitchClips.cpp

    PYMODULE_FILES
        __init__.py
)

pxr_test_scripts(
    testenv/testUsdUtilsAuthoring.py 
    testenv/testUsdUtilsDependencyExtractor.py 
    testenv/testUsdUtilsFlattenLayerStack.py
    testenv/testUsdUtilsIntrospection.py 
    testenv/testUsdUtilsPipeline.py 
    testenv/testUsdUtilsStageCache.py 
    testenv/testUsdUtilsStitchClips.py 
    testenv/testUsdUtilsStitch.py 
    testenv/testUsdUtilsVarSelsSessionLayer.py 
)

pxr_install_test_dir(
    SRC testenv/testUsdUtilsAuthoring
    DEST testUsdUtilsAuthoring
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
    SRC testenv/testUsdUtilsPipeline
    DEST testUsdUtilsPipeline
)

pxr_install_test_dir(
    SRC testenv/testUsdUtilsIntrospection
    DEST testUsdUtilsIntrospection
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

pxr_register_test(testUsdUtilsPipeline
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdUtilsPipeline"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdUtilsIntrospection
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdUtilsIntrospection"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdUtilsStageCache
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdUtilsStageCache"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdUtilsVarSelsSessionLayer
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdUtilsVarSelsSessionLayer"
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

