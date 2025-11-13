set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usdShade)

pxr_library(usdShade
    INCLUDE_SCHEMA_FILES

    LIBRARIES
        tf
        vt
        js
        sdf
        sdr
        usd
        usdGeom
        TBB::tbb
    
    PUBLIC_CLASSES
        connectableAPIBehavior
        input
        output
        shaderDefParser
        shaderDefUtils
        udimUtils
        utils

    PUBLIC_HEADERS
        api.h
        types.h

    PYMODULE_CPPFILES
        wrapInput.cpp
        wrapOutput.cpp
        wrapShaderDefParser.cpp
        wrapShaderDefUtils.cpp
        wrapTypes.cpp
        wrapUdimUtils.cpp
        wrapUtils.cpp

    PYMODULE_FILES
        __init__.py

    DOXYGEN_FILES
        overview.dox
)

pxr_build_test(testUsdShadeHasConnectableAPI
    LIBRARIES
        tf
        usdShade
        usdGeom
    CPPFILES
        testenv/testUsdShadeHasConnectableAPI.cpp
)

pxr_test_scripts(
    testenv/testUsdShadeBinding.py
    testenv/testUsdShadeConnectability.py
    testenv/testUsdShadeConnectableAPIBehavior.py
    testenv/testUsdShadeCoordSysAPI.py
    testenv/testUsdShadeCoordSysAPIDeprecated.py
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
    testenv/testUsdShadeUdimUtils.py
)

pxr_register_test(testUsdShadeBinding
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdShadeBinding"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdShadeConnectableAPIBehavior
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdShadeConnectableAPIBehavior"
    TESTENV testenv/testUsdShadeConnectableAPIBehavior
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdShadeCoordSysAPI
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdShadeCoordSysAPI"
    TESTENV testenv/testUsdShadeCoordSysAPI
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdShadeGetValueProducingAttribute
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdShadeGetValueProducingAttribute"
    TESTENV testenv/testUsdShadeGetValueProducingAttribute
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdShadeMaterialAuthoring
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdShadeMaterialAuthoring"
    TESTENV testenv/testUsdShadeMaterialAuthoring
    DIFF_COMPARE char_shading.usda char_shading_compact.usda
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdShadeMaterialBinding
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdShadeMaterialBinding"
    TESTENV testenv/testUsdShadeMaterialBinding
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
    TESTENV testenv/testUsdShadeMaterialBindFaceSubset
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
    TESTENV testenv/testUsdShadeShaderDef
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
    TESTENV testenv/testUsdShadeMaterialBaseMaterial
    EXPECTED_RETURN_CODE 0
    DIFF_COMPARE test_base_material_specializes.usda
)

pxr_register_test(testUsdShadeMaterialSpecializesBaseComposition
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdShadeMaterialSpecializesBaseComposition"
    TESTENV testenv/testUsdShadeMaterialSpecializesBaseComposition
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

pxr_register_test(testUsdShadeHasConnectableAPI
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdShadeHasConnectableAPI"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdShadeConnectability_OldEncoding
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdShadeConnectability"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdShadeUdimUtils
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdShadeUdimUtils"
    TESTENV testenv/testUsdShadeUdimUtils
    EXPECTED_RETURN_CODE 0
)
