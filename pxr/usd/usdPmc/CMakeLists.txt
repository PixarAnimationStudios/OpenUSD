set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usdPmc)

pxr_library(usdPmc
    LIBRARIES
        # List dependency libraries
        tf
        sdf
        usd
        usdGeom
        usdUtils
        Pmc::Pmc

    INCLUDE_DIRS

    CPPFILES
        usdPmcEncoder.cpp
        usdPmcFileFormat.cpp
        usdPmcEncodeSession.cpp

    PRIVATE_CLASSES
        usdPmcDecoder

    PYMODULE_CPPFILES
        module.cpp
        wrapUsdPmcEncoder.cpp

    PYMODULE_FILES
        __init__.py

    RESOURCE_FILES
        plugInfo.json

    PUBLIC_HEADERS
        api.h
        usdPmcEncoder.h
)

pxr_test_scripts(
    testenv/testUsdPmc.py
    testenv/testUsdPmcMeshEncoder.py
)

pxr_register_test(testUsdPmc
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdPmc"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdPmcMeshEncoderRelativePaths
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdPmcMeshEncoder relativePaths/asset.usda relativePathsEncoded.usda"
    TESTENV testenv/testUsdPmcMeshEncoder
    EXPECTED_RETURN_CODE 0
    DIFF_COMPARE relativePathsEncoded.usda
)
