set(PXR_PACKAGE exec)

pxr_build_test(definingComputations
    LIBRARIES
        exec
        execUsd
        plug
        sdf
        tf
        usd
        vdf
    CPPFILES
        definingComputations.cpp
)

pxr_register_test(testDefiningComputationsExample
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/definingComputations"
    TESTENV testenv/testDefiningComputationsExample
    EXPECTED_RETURN_CODE 0
)
