set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usdText)

set(optionalPublicClasses "")
if (PXR_ENABLE_TEXT_SUPPORT)
    list(APPEND optionalPublicClasses simpleText markupText)
    list(APPEND optionalPyModuleFiles wrapSimpleText.cpp wrapMarkupText.cpp)
endif()

pxr_library(usdText
    LIBRARIES
        tf
        usd
        usdGeom

    PUBLIC_CLASSES
        textStyleAPI
        textStyle
        textLayout
        columnStyle
        columnStyleAPI
        paragraphStyle
        paragraphStyleAPI
        tokens
        ${optionalPublicClasses}

    PUBLIC_HEADERS
        api.h

    PYTHON_CPPFILES
        moduleDeps.cpp

    PYMODULE_CPPFILES
        module.cpp
        wrapTextStyleAPI.cpp
        wrapTextStyle.cpp
        wrapTextLayout.cpp
        wrapColumnStyle.cpp
        wrapColumnStyleAPI.cpp
        wrapParagraphStyle.cpp
        wrapParagraphStyleAPI.cpp
        wrapTokens.cpp
        ${optionalPyModuleFiles}

    PYMODULE_FILES
        __init__.py

    RESOURCE_FILES
        plugInfo.json
        generatedSchema.usda
        schema.usda:usdText/schema.usda
)

if (PXR_ENABLE_TEXT_SUPPORT)
pxr_test_scripts(
    testenv/testUsdTextSimpleText.py
    testenv/testUsdTextMarkupText.py
    testenv/testUsdTextSchemata.py
)

pxr_install_test_dir(
    SRC testenv/testUsdTextSimpleText
    DEST testUsdTextSimpleText
)

pxr_install_test_dir(
    SRC testenv/testUsdTextMarkupText
    DEST testUsdTextMarkupText
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

pxr_register_test(testUsdTextMarkupText
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdTextMarkupText"
    EXPECTED_RETURN_CODE 0
)
endif()