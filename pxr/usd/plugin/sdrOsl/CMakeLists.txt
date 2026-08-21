set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE sdrOsl)

if(NOT PXR_ENABLE_OSL_SUPPORT)
    return()
endif()

pxr_plugin(sdrOsl
    LIBRARIES
        gf
        tf
        vt
        ar
        sdr
        ${OSL_QUERY_LIBRARY}
        OpenImageIO::OpenImageIO

    INCLUDE_DIRS
        ${OSL_INCLUDE_DIR}

    PUBLIC_CLASSES
        oslParser

    PRIVATE_HEADERS
        api.h

    RESOURCE_FILES
        plugInfo.json
)

pxr_test_scripts(
    testenv/testOslParser.py
)

pxr_register_test(testOslParser
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testOslParser"
    TESTENV testenv/testOslParser.testenv
    EXPECTED_RETURN_CODE 0
)
