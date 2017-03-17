set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usdRi)

pxr_shared_library(usdRi
    LIBRARIES
        tf
        vt
        sdf
        usd
        usdShade
        usdGeom
        ${Boost_PYTHON_LIBRARY}

    INCLUDE_DIRS
        ${Boost_INCLUDE_DIRS}

    PUBLIC_CLASSES
        tokens
        statements
        lookAPI
        rmanUtilities
        rslShader
        risIntegrator
        risBxdf
        risObject
        risOslPattern
        risPattern

    PUBLIC_HEADERS
        api.h

    CPPFILES
        moduleDeps.cpp
        typeUtils

    PYMODULE_CPPFILES
        module.cpp
        wrapLookAPI.cpp
        wrapRmanUtilities.cpp
        wrapRisBxdf.cpp
        wrapRisIntegrator.cpp
        wrapRisObject.cpp
        wrapRisOslPattern.cpp
        wrapRisPattern.cpp
        wrapRslShader.cpp
        wrapStatements.cpp
        wrapTokens.cpp

    PYTHON_FILES
        __init__.py

    RESOURCE_FILES
        plugInfo.json
        generatedSchema.usda
)

pxr_test_scripts(
    testenv/testUsdRiSchemata.py
    testenv/testUsdRiUtilities.py
)

pxr_register_test(testUsdRiSchemata
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdRiSchemata"  
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdRiUtilities
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdRiUtilities"  
    EXPECTED_RETURN_CODE 0
)
