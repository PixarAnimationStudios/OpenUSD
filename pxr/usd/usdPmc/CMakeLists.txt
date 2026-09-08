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
        # List non-schema cpp files
        usdPmcDecoder.cpp
        usdPmcEncoder.cpp
        usdPmcFileFormat.cpp
        usdPmcEncodeSession.cpp

    PYMODULE_CPPFILES
        module.cpp
        wrapUsdPmcEncoder.cpp

    PYMODULE_FILES
        __init__.py

    RESOURCE_FILES
        plugInfo.json

    PUBLIC_HEADERS
        api.h
        usdPmcEncoder.hpp
)

pxr_test_scripts(
    testenv/testUsdPmc.py
)

pxr_register_test(testUsdPmc
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdPmc"
    EXPECTED_RETURN_CODE 0
    PRE_PATH
        ${CMAKE_INSTALL_PREFIX}/bin
)
