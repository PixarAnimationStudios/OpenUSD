set(PXR_PACKAGE invertibleRigsExampleAuthoringCode)

pxr_plugin(${PXR_PACKAGE}
    LIBRARIES
        arch
        exec
        execIr
        execUsd
        gf
        sdf
        tf
        ts
        usd

    PUBLIC_HEADERS
        api.h

    PUBLIC_CLASSES
        authoring
        query

    PYMODULE_CPPFILES
        module.cpp
        wrapAuthoring.cpp
        wrapQuery.cpp

    PYMODULE_FILES
        __init__.py
)

pxr_test_scripts(
    testenv/testSwitchCompensation.py
)

pxr_register_test(testSwitchCompensation
    PYTHON
    PRE_PATH
        ${CMAKE_INSTALL_PREFIX}/share/exec/examples/plugin
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testSwitchCompensation"
    TESTENV testenv/testSwitchCompensation
    EXPECTED_RETURN_CODE 0
)
