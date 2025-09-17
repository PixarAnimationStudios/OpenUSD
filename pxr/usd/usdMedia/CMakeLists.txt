set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usdMedia)

pxr_library(usdMedia
    INCLUDE_SCHEMA_FILES

    LIBRARIES
        tf
        vt
        sdf
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
    testenv/testUsdMediaAssetPreviewsAPI.py
    testenv/testUsdMediaSpatialAudio.py
)

pxr_register_test(testUsdMediaAssetPreviewsAPI
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdMediaAssetPreviewsAPI"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdMediaSpatialAudio
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdMediaSpatialAudio"
    EXPECTED_RETURN_CODE 0
)
