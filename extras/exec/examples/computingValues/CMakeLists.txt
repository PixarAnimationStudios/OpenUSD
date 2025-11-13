set(PXR_PACKAGE exec)

pxr_build_test(computingValues
    LIBRARIES
        gf
        execGeom
        execUsd
        sdf
        tf
        usd
        vt
    CPPFILES
        computingValues.cpp
)

pxr_register_test(testComputingValuesExample
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/computingValues"
    TESTENV testenv/testComputingValuesExample
    EXPECTED_RETURN_CODE 0
)
