set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usdShade)

pxr_library(usdShade
    LIBRARIES
        tf
        vt
        sdf
        ndr
        sdr
        usd
        usdGeom

    PUBLIC_CLASSES
        connectableAPI
        coordSysAPI
        input
        material
        materialBindingAPI
        output
        shader
        shaderDefParser
        shaderDefUtils
        nodeGraph
        tokens
        utils

    PUBLIC_HEADERS
        api.h

    PYTHON_CPPFILES
        moduleDeps.cpp

    PYMODULE_CPPFILES
        module.cpp
        wrapConnectableAPI.cpp
        wrapCoordSysAPI.cpp
        wrapInput.cpp
        wrapMaterial.cpp
        wrapMaterialBindingAPI.cpp
        wrapOutput.cpp
        wrapShader.cpp
        wrapShaderDefParser.cpp
        wrapShaderDefUtils.cpp
        wrapNodeGraph.cpp
        wrapTokens.cpp
        wrapUtils.cpp

    PYMODULE_FILES
        __init__.py

    RESOURCE_FILES
        plugInfo.json
        generatedSchema.usda
        schema.usda:usdShade/schema.usda
)

pxr_test_scripts(
    testenv/testUsdShadeBinding.py
    testenv/testUsdShadeConnectability.py
    testenv/testUsdShadeCoordSysAPI.py
    testenv/testUsdShadeGetValueProducingAttribute.py
    testenv/testUsdShadeInterfaceInputConsumers.py
    testenv/testUsdShadeMaterialBinding.py
    testenv/testUsdShadeMaterialOutputs.py
    testenv/testUsdShadeMaterialBindFaceSubset.py
    testenv/testUsdShadeMaterialAuthoring.py
    testenv/testUsdShadeMaterialBaseMaterial.py
    testenv/testUsdShadeMaterialSpecializesBaseComposition.py
    testenv/testUsdShadeNodeGraphs.py
    testenv/testUsdShadeShaders.py
    testenv/testUsdShadeShaderDef.py
)

pxr_install_test_dir(
    SRC testenv/testUsdShadeCoordSysAPI
    DEST testUsdShadeCoordSysAPI
)

pxr_install_test_dir(
    SRC testenv/testUsdShadeGetValueProducingAttribute
    DEST testUsdShadeGetValueProducingAttribute
)

pxr_install_test_dir(
    SRC testenv/testUsdShadeMaterialBinding
    DEST testUsdShadeMaterialBinding
)

pxr_install_test_dir(
    SRC testenv/testUsdShadeMaterialBindFaceSubset
    DEST testUsdShadeMaterialBindFaceSubset
)
pxr_install_test_dir(
    SRC testenv/testUsdShadeMaterialAuthoring
    DEST testUsdShadeMaterialAuthoring
)

pxr_install_test_dir(
    SRC testenv/testUsdShadeMaterialBaseMaterial
    DEST testUsdShadeMaterialBaseMaterial
)

pxr_install_test_dir(
    SRC testenv/testUsdShadeMaterialSpecializesBaseComposition
    DEST testUsdShadeMaterialSpecializesBaseComposition
)

pxr_install_test_dir(
    SRC testenv/testUsdShadeShaderDef
    DEST testUsdShadeShaderDef
)

pxr_register_test(testUsdShadeBinding
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdShadeBinding"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdShadeCoordSysAPI
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdShadeCoordSysAPI"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdShadeGetValueProducingAttribute
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdShadeGetValueProducingAttribute"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdShadeMaterialAuthoring
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdShadeMaterialAuthoring"
    DIFF_COMPARE char_shading.usda char_shading_compact.usda
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdShadeMaterialBinding
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdShadeMaterialBinding"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdShadeMaterialOutputs
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdShadeMaterialOutputs"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdShadeMaterialBindFaceSubset
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdShadeMaterialBindFaceSubset"
    DIFF_COMPARE SphereWithMaterialBind.usda
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdShadeShaders
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdShadeShaders"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdShadeShaderDef
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdShadeShaderDef"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdShadeNodeGraphs
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdShadeNodeGraphs"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdShadeMaterialBaseMaterial
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdShadeMaterialBaseMaterial"
    EXPECTED_RETURN_CODE 0
    DIFF_COMPARE test_base_material_specializes.usda
)

pxr_register_test(testUsdShadeMaterialSpecializesBaseComposition
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdShadeMaterialSpecializesBaseComposition"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdShadeInterfaceInputConsumers
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdShadeInterfaceInputConsumers"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdShadeConnectability
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdShadeConnectability"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdShadeConnectability_OldEncoding
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdShadeConnectability"
    EXPECTED_RETURN_CODE 0
)
