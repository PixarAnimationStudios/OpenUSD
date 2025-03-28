set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usdRi)

pxr_library(usdRi
    INCLUDE_SCHEMA_FILES

    LIBRARIES
        tf
        vt
        sdf
        usd
        usdShade
        usdGeom

    PUBLIC_CLASSES
        rmanUtilities

    PUBLIC_HEADERS
        api.h

    PRIVATE_CLASSES
        typeUtils

    PYMODULE_CPPFILES
        wrapRmanUtilities.cpp

    PYMODULE_FILES
        __init__.py

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
