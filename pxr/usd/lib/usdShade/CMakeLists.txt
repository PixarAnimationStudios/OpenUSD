set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usdShade)

pxr_shared_library(usdShade
    LIBRARIES
        tf
        vt
        sdf
        usd
        usdGeom

    PUBLIC_CLASSES
        connectableAPI
        debugCodes
        input
        interfaceAttribute
        look
        material
        output
        parameter
        pShader
        pShaderUtils
        shader
        nodeGraph
        tokens
        utils

    PUBLIC_HEADERS
        api.h

    CPPFILES
        moduleDeps.cpp

    PYMODULE_CPPFILES
        module.cpp
        wrapConnectableAPI.cpp
        wrapInput.cpp
        wrapInterfaceAttribute.cpp
        wrapLook.cpp
        wrapMaterial.cpp
        wrapOutput.cpp
        wrapParameter.cpp
        wrapPShader.cpp
        wrapShader.cpp
        wrapNodeGraph.cpp
        wrapTokens.cpp
        wrapUtils.cpp

    PYTHON_FILES
        __init__.py

    RESOURCE_FILES
        plugInfo.json
        generatedSchema.usda
)

pxr_test_scripts(
    testenv/testUsdShadeBinding.py
    testenv/testUsdShadeConnectability.py
    testenv/testUsdShadeInterfaceInputConsumers.py
    testenv/testUsdShadeMaterialFaceSet.py
    testenv/testUsdShadeMaterialAuthoring.py
    testenv/testUsdShadeMaterialBaseMaterial.py
    testenv/testUsdShadeNodeGraphs.py
    testenv/testUsdShadeShaders.py
)

pxr_install_test_dir(
    SRC testenv/testUsdShadeMaterialFaceSet
    DEST testUsdShadeMaterialFaceSet
)

pxr_install_test_dir(
    SRC testenv/testUsdShadeMaterialAuthoring
    DEST testUsdShadeMaterialAuthoring
)

pxr_install_test_dir(
    SRC testenv/testUsdShadeMaterialBaseMaterial
    DEST testUsdShadeMaterialBaseMaterial_derives
)

pxr_install_test_dir(
    SRC testenv/testUsdShadeMaterialBaseMaterial
    DEST testUsdShadeMaterialBaseMaterial_specializes
)

pxr_register_test(testUsdShadeBinding
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdShadeBinding"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdShadeMaterialAuthoring
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdShadeMaterialAuthoring"
    DIFF_COMPARE char_shading.usda char_shading_compact.usda
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdShadeMaterialFaceSet
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdShadeMaterialFaceSet"
    DIFF_COMPARE SphereWithFaceSets.usda
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdShadeShaders
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdShadeShaders"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdShadeNodeGraphs
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdShadeNodeGraphs"
    EXPECTED_RETURN_CODE 0
    ENV
        USD_SHADE_WRITE_NEW_ENCODING=1
)

pxr_register_test(testUsdShadeMaterialBaseMaterial_derives
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdShadeMaterialBaseMaterial"
    EXPECTED_RETURN_CODE 0
    DIFF_COMPARE test_base_material.usda
    ENV
        USD_USE_LEGACY_BASE_MATERIAL=1
)

pxr_register_test(testUsdShadeMaterialBaseMaterial_specializes
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdShadeMaterialBaseMaterial"
    EXPECTED_RETURN_CODE 0
    DIFF_COMPARE test_base_material_specializes.usda
    ENV
        USD_USE_LEGACY_BASE_MATERIAL=0
)

pxr_register_test(testUsdShadeInterfaceInputConsumers_OldEncoding
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdShadeInterfaceInputConsumers"
    EXPECTED_RETURN_CODE 0
    ENV
        USD_SHADE_ENABLE_BIDIRECTIONAL_INTERFACE_CONNECTIONS=1
)

pxr_register_test(testUsdShadeInterfaceInputConsumers
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdShadeInterfaceInputConsumers"
    EXPECTED_RETURN_CODE 0
    ENV
        USD_SHADE_WRITE_NEW_ENCODING=1
)

pxr_register_test(testUsdShadeConnectability
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdShadeConnectability"
    EXPECTED_RETURN_CODE 0
    ENV
        USD_SHADE_WRITE_NEW_ENCODING=1
        USD_SHADE_READ_OLD_ENCODING=0
)

pxr_register_test(testUsdShadeConnectability_OldEncoding
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdShadeConnectability"
    EXPECTED_RETURN_CODE 0
)
