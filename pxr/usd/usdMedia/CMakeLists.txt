set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usdMedia)

pxr_library(usdMedia
    LIBRARIES
        tf
        vt
        sdf
        usd
        usdGeom

    PUBLIC_CLASSES
        assetPreviewsAPI
        spatialAudio
        tokens

    PUBLIC_HEADERS
        api.h

    PYTHON_CPPFILES
        moduleDeps.cpp

    PYMODULE_CPPFILES
        module.cpp
        wrapAssetPreviewsAPI.cpp
        wrapSpatialAudio.cpp
        wrapTokens.cpp

    PYMODULE_FILES 
        __init__.py

    RESOURCE_FILES
        plugInfo.json
        generatedSchema.usda
        schema.usda:usdMedia/schema.usda

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
