set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usdVol)

pxr_library(usdVol
    INCLUDE_SCHEMA_FILES

    LIBRARIES
        tf
        usd
        usdGeom

    PUBLIC_HEADERS
        api.h

    PYMODULE_FILES
        __init__.py

    DOXYGEN_FILES
        overview.dox
)

pxr_test_scripts(
    testenv/testUsdVolVolume.py
)

pxr_register_test(testUsdVolVolume
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdVolVolume"
    EXPECTED_RETURN_CODE 0
)


