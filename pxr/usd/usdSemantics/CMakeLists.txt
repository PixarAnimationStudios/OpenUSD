set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usdSemantics)

pxr_library(usdSemantics
    INCLUDE_SCHEMA_FILES

    LIBRARIES
        tf
        vt
        usd

    PUBLIC_CLASSES
        labelsQuery

    PUBLIC_HEADERS
        api.h

    PYMODULE_CPPFILES
        wrapLabelsQuery.cpp

    PYMODULE_FILES
        __init__.py

    DOXYGEN_FILES
        overview.md
)

pxr_test_scripts(
    testenv/testUsdSemanticsLabelsAPI.py
    testenv/testUsdSemanticsLabelsQuery.py
)

pxr_register_test(testUsdSemanticsLabelsAPI
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdSemanticsLabelsAPI"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdSemanticsQuery
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdSemanticsLabelsQuery"
    EXPECTED_RETURN_CODE 0
)