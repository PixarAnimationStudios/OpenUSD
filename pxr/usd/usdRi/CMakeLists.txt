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
        ${Boost_PYTHON_LIBRARY}

    INCLUDE_DIRS
        ${Boost_INCLUDE_DIRS}

    PUBLIC_CLASSES
        materialAPI
        renderPassAPI
        rmanUtilities
        splineAPI
        statementsAPI
        tokens

    PUBLIC_HEADERS
        api.h

    PYTHON_CPPFILES
        moduleDeps.cpp

    PRIVATE_CLASSES
        typeUtils

    PYMODULE_CPPFILES
        module.cpp
        wrapMaterialAPI.cpp
        wrapRenderPassAPI.cpp
        wrapRmanUtilities.cpp
        wrapSplineAPI.cpp
        wrapStatementsAPI.cpp
        wrapTokens.cpp

    PYMODULE_FILES
        __init__.py

    RESOURCE_FILES
        plugInfo.json
        generatedSchema.usda
        schema.usda:usdRi/schema.usda

    DOXYGEN_FILES
        overview.dox
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
