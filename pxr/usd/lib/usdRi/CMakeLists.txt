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
        splineAPI
        statementsAPI
        textureAPI
        tokens

    PUBLIC_HEADERS
        api.h

    PYTHON_CPPFILES
        moduleDeps.cpp

    CPPFILES
        typeUtils

    PYMODULE_CPPFILES
        module.cpp
        wrapLightAPI.cpp
        wrapLightFilterAPI.cpp
        wrapLightPortalAPI.cpp
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
        wrapSplineAPI.cpp
        wrapStatementsAPI.cpp
        wrapTextureAPI.cpp
        wrapTokens.cpp

    PYMODULE_FILES
        __init__.py

    RESOURCE_FILES
        plugInfo.json
        generatedSchema.usda
        schema.usda:usdRi/schema.usda
)

pxr_test_scripts(
    testenv/testUsdRiSchemata.py
    testenv/testUsdRiSplineAPI.py
    testenv/testUsdRiUtilities.py
)

pxr_register_test(testUsdRiSchemata
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdRiSchemata"  
    EXPECTED_RETURN_CODE 0
    ENV
        USD_RI_WRITE_BXDF_OUTPUT=0
)

pxr_register_test(testUsdRiSplineAPI
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdRiSplineAPI"  
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdRiUtilities
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdRiUtilities"  
    EXPECTED_RETURN_CODE 0
)
