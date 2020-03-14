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
        spatialAudio
        tokens

    PUBLIC_HEADERS
        api.h

    PYTHON_CPPFILES
        moduleDeps.cpp

    PYMODULE_CPPFILES
        module.cpp
        wrapSpatialAudio.cpp
        wrapTokens.cpp

    PYMODULE_FILES 
        __init__.py

    RESOURCE_FILES
        plugInfo.json
        generatedSchema.usda
        schema.usda:usdMedia/schema.usda
)

pxr_test_scripts(
    testenv/testUsdMediaSpatialAudio.py
)

pxr_register_test(testUsdMediaSpatialAudio
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdMediaSpatialAudio"
    EXPECTED_RETURN_CODE 0
)
