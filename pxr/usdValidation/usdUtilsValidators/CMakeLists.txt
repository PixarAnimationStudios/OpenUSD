set(PXR_PREFIX pxr/usdValidation)
set(PXR_PACKAGE usdUtilsValidators)

pxr_library(usdUtilsValidators
    LIBRARIES
        tf
        plug
        sdf
        usd
        usdUtils
        usdValidation

    PUBLIC_CLASSES
        validatorTokens
        
    PUBLIC_HEADERS
        api.h

    CPPFILES
        validators.cpp

    RESOURCE_FILES
        plugInfo.json
)

pxr_build_test(testUsdUtilsValidators
    LIBRARIES
        ar
        arch
        tf
        sdf
        usd
        usdGeom
        usdShade
        usdValidation
        usdUtils
        usdUtilsValidators
    CPPFILES
        testenv/testUsdUtilsValidators.cpp
)

pxr_install_test_dir(
    SRC testenv/testUsdUtilsValidators
    DEST testUsdUtilsValidators
)

pxr_register_test(testUsdUtilsValidators
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdUtilsValidators"
    EXPECTED_RETURN_CODE 0
)

