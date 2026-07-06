set(PXR_PREFIX pxr/exec)
set(PXR_PACKAGE execIr)

# TODO: Currently, generated schema code only works with hidden symbol
# visibility on Windows; therefore, we disable it on Linux for now. (Note that
# OpenExec doesn't yet support hidden symbol visibility on Apple platforms and
# that CMAKE_CXX_VISIBILITY_PRESET has no effect if the compiler is MSVC.
unset(CMAKE_CXX_VISIBILITY_PRESET)

pxr_library(execIr
    INCLUDE_SCHEMA_FILES

    LIBRARIES
        execGeom
        execUsd
        gf
        sdf
        tf
        usd
        vt
        
    PUBLIC_HEADERS
        api.h

    PUBLIC_CLASSES
        computations
        controllerBuilder
        types

    PRIVATE_HEADERS

    PRIVATE_CLASSES
        utils

    CPPFILES
        fkControllerComputations.cpp
        switchControllerComputations.cpp
        xformableComputations.cpp

    PYMODULE_FILES
        __init__.py

    DOXYGEN_FILES
        README.md
)

#
# Tests
#

pxr_build_test(testExecIrController
    LIBRARIES
        exec
        execIr
        execUsd
        plug
        sdf
        tf
        usd
        vdf
    CPPFILES
        testenv/testExecIrController.cpp
)

pxr_register_test(testExecIrController
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testExecIrController"
    TESTENV testenv/testExecIrController
    EXPECTED_RETURN_CODE 0
)

pxr_build_test(testExecIrFkController
    LIBRARIES
        exec
        execIr
        execUsd
        gf
        sdf
        tf
        usd
    CPPFILES
        testenv/testExecIrFkController.cpp
)

pxr_register_test(testExecIrFkController
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testExecIrFkController"
    EXPECTED_RETURN_CODE 0
)

pxr_build_test(testExecIrJointScope
    LIBRARIES
        exec
        execIr
        execUsd
        gf
        sdf
        tf
        usd
        usdGeom
    CPPFILES
        testenv/testExecIrJointScope.cpp
)

pxr_register_test(testExecIrJointScope
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testExecIrJointScope"
    EXPECTED_RETURN_CODE 0
)

pxr_build_test(testExecIrSwitchController
    LIBRARIES
        exec
        execIr
        execUsd
        gf
        sdf
        tf
        usd
    CPPFILES
        testenv/testExecIrSwitchController.cpp
)

pxr_register_test(testExecIrSwitchController
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testExecIrSwitchController"
    EXPECTED_RETURN_CODE 0
)

pxr_build_test(testExecIrSwitchCompensation
    LIBRARIES
        exec
        execIr
        execUsd
        gf
        sdf
        tf
        ts
        usd
    CPPFILES
        testenv/testExecIrSwitchCompensation.cpp
)

pxr_register_test(testExecIrSwitchCompensation
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testExecIrSwitchCompensation"
    EXPECTED_RETURN_CODE 0
)
