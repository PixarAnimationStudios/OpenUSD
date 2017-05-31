set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usdRi)

pxr_library(usdRi
    LIBRARIES
        tf
        vt
        sdf
        usd
        usdShade
        usdGeom
        usdLux
        ${Boost_PYTHON_LIBRARY}

    INCLUDE_DIRS
        ${Boost_INCLUDE_DIRS}

    PUBLIC_CLASSES
        lightAPI
        lightFilterAPI
        lightPortalAPI
        lookAPI
        materialAPI
        pxrAovLight
        pxrBarnLightFilter
        pxrCookieLightFilter
        pxrEnvDayLight
        pxrIntMultLightFilter
        pxrRampLightFilter
        pxrRodLightFilter
        risBxdf
        risIntegrator
        risObject
        risOslPattern
        risPattern
        rmanUtilities
        rslShader
        statements
        tokens

    PUBLIC_HEADERS
        api.h

    CPPFILES
        moduleDeps.cpp
        typeUtils

    PYMODULE_CPPFILES
        module.cpp
        wrapLightAPI.cpp
        wrapLightFilterAPI.cpp
        wrapLightPortalAPI.cpp
        wrapLookAPI.cpp
        wrapMaterialAPI.cpp
        wrapPxrAovLight.cpp
        wrapPxrBarnLightFilter.cpp
        wrapPxrCookieLightFilter.cpp
        wrapPxrEnvDayLight.cpp
        wrapPxrIntMultLightFilter.cpp
        wrapPxrRampLightFilter.cpp
        wrapPxrRodLightFilter.cpp
        wrapRisBxdf.cpp
        wrapRisIntegrator.cpp
        wrapRisObject.cpp
        wrapRisOslPattern.cpp
        wrapRisPattern.cpp
        wrapRmanUtilities.cpp
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
