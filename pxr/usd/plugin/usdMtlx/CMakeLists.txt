set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usdMtlx)

if (MATERIALX_STDLIB_DIR)
    add_definitions(-DPXR_MATERIALX_STDLIB_DIR="${MATERIALX_STDLIB_DIR}")
endif()

pxr_plugin(usdMtlx
    LIBRARIES
        arch
        gf
        ndr
        sdf
        sdr
        tf
        vt
        usd
        usdGeom
        usdShade
        usdUI
        usdUtils
        MaterialXCore
        MaterialXFormat

    CPPFILES
        debugCodes.cpp
        discovery.cpp
        parser.cpp

    PRIVATE_CLASSES
        backdoor
        fileFormat
        reader
        utils

    PRIVATE_HEADERS
        api.h
	debugCodes.h

    PYTHON_CPPFILES
        moduleDeps.cpp

    PYMODULE_CPPFILES
        module.cpp
        wrapBackdoor.cpp

    PYMODULE_FILES
        __init__.py

    RESOURCE_FILES
        plugInfo.json

    DOXYGEN_FILES
        overview.dox
)

pxr_test_scripts(
    testenv/testUsdMtlxDiscovery.py
    testenv/testUsdMtlxFileFormat.py
    testenv/testUsdMtlxParser.py
)

pxr_install_test_dir(
    SRC testenv/testUsdMtlxDiscovery.testenv
    DEST testUsdMtlxDiscovery
)

pxr_install_test_dir(
    SRC testenv/testUsdMtlxFileFormat.testenv
    DEST testUsdMtlxFileFormat
)

pxr_install_test_dir(
    SRC testenv/testUsdMtlxParser.testenv
    DEST testUsdMtlxParser
)

pxr_register_test(testUsdMtlxDiscovery
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdMtlxDiscovery"
    EXPECTED_RETURN_CODE 0
)

if (PXR_USE_AR_2)
    pxr_register_test(testUsdMtlxFileFormat
        PYTHON
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdMtlxFileFormat"
        DIFF_COMPARE Looks.usda NodeGraphs.usda usd_preview_surface_gold.usda Include.usda Include_From_Usdz.usda
        EXPECTED_RETURN_CODE 0
    )
else()
    pxr_register_test(testUsdMtlxFileFormat
        PYTHON
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdMtlxFileFormat"
        DIFF_COMPARE Looks.usda NodeGraphs.usda
        EXPECTED_RETURN_CODE 0
    )
endif()

pxr_register_test(testUsdMtlxParser
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdMtlxParser"
    EXPECTED_RETURN_CODE 0
)
