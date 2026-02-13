set(PXR_PREFIX )
set(PXR_PACKAGE execIr)

pxr_library(execIr
    INCLUDE_SCHEMA_FILES

    LIBRARIES
        gf
        tf
        execUsd
        
    PUBLIC_HEADERS
        api.h

    PUBLIC_CLASSES
        tokens
        types

    PRIVATE_HEADERS

    PRIVATE_CLASSES
        utils

    CPPFILES
        fkController.cpp

    RESOURCE_FILES
        plugInfo.json
)

#
# Tests
#

pxr_build_test(testExecIrBasic
    LIBRARIES
        gf
        tf
        exec
        execIr
        execUsd
        sdf
        usd
    CPPFILES
        testenv/testExecIrBasic.cpp
)

pxr_register_test(testExecIrBasic
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testExecIrBasic"
    EXPECTED_RETURN_CODE 0
)
