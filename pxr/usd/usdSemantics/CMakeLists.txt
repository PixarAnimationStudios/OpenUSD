set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usdSemantics)

pxr_library(usdSemantics
    LIBRARIES
        tf
        vt
        usd

    PUBLIC_CLASSES
        labelsAPI
        labelsQuery
        tokens

    PUBLIC_HEADERS
        api.h

    PYTHON_CPPFILES
        moduleDeps.cpp

    PYMODULE_CPPFILES
        module.cpp
        wrapLabelsAPI.cpp
        wrapLabelsQuery.cpp
        wrapTokens.cpp

    PYMODULE_FILES
        __init__.py

    RESOURCE_FILES
        plugInfo.json
        generatedSchema.usda
        schema.usda:usdSemantics/schema.usda

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