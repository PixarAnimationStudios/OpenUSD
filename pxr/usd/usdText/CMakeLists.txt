set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usdText)

pxr_library(usdText
    LIBRARIES
        tf
        usd
        usdGeom

    PUBLIC_CLASSES
        textStyleAPI
        textStyle
        simpleText
        tokens

    PUBLIC_HEADERS
        api.h

    PYTHON_CPPFILES
        moduleDeps.cpp

    PYMODULE_CPPFILES
        module.cpp
        wrapTextStyleAPI.cpp
        wrapTextStyle.cpp
        wrapSimpleText.cpp
        wrapTokens.cpp
        ${optionalPyModuleFiles}

    PYMODULE_FILES
        __init__.py

    RESOURCE_FILES
        plugInfo.json
        generatedSchema.usda
        schema.usda:usdText/schema.usda
)

pxr_test_scripts(
    testenv/testUsdTextSimpleText.py
    testenv/testUsdTextSchemata.py
)

pxr_install_test_dir(
    SRC testenv/testUsdTextSimpleText
    DEST testUsdTextSimpleText
)

pxr_register_test(testUsdTextSchemata
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdTextSchemata"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdTextSimpleText
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdTextSimpleText"
    EXPECTED_RETURN_CODE 0
)