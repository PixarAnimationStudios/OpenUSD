set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usdLux)

pxr_library(usdLux
    INCLUDE_SCHEMA_FILES

    LIBRARIES
        tf
        vt
        sdf
        sdr
        usd
        usdGeom
        usdShade

    PUBLIC_CLASSES
        blackbody

    PUBLIC_HEADERS
        api.h

    PRIVATE_CLASSES
        discoveryPlugin
        lightDefParser

    PYMODULE_FILES 
        __init__.py

    DOXYGEN_FILES
        overview.dox
)

pxr_test_scripts(
    testenv/testUsdLuxLight.py
    testenv/testUsdLuxLinkingAPI.py
    testenv/testUsdLuxLightListAPI.py
)

pxr_register_test(testUsdLuxLight
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdLuxLight"
    EXPECTED_RETURN_CODE 0
)

pxr_install_test_dir(
    DEST testUsdLuxLinkingAPI
    SRC testenv/testUsdLuxLinkingAPI
)

pxr_register_test(testUsdLuxLinkingAPI
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdLuxLinkingAPI"
    EXPECTED_RETURN_CODE 0
)

pxr_install_test_dir(
    DEST testUsdLuxLightListAPI
    SRC testenv/testUsdLuxLightListAPI
)

pxr_register_test(testUsdLuxLightListAPI
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdLuxLightListAPI"
    EXPECTED_RETURN_CODE 0
)


