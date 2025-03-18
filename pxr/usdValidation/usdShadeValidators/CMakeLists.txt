set(PXR_PREFIX pxr/usdValidation)
set(PXR_PACKAGE usdShadeValidators)

pxr_library(usdShadeValidators
    LIBRARIES
        tf
        plug
        sdf
        usd
        sdr
        usdShade
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

pxr_build_test(testUsdShadeValidators
    LIBRARIES
        tf
        sdr
        usd
        usdValidation
        usdShade
        usdShadeValidators
    CPPFILES
        testenv/testUsdShadeValidators.cpp
)

pxr_install_test_dir(
    SRC testenv/testUsdShadeValidators
    DEST testUsdShadeValidators
)

pxr_register_test(testUsdShadeValidators
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdShadeValidators"
    EXPECTED_RETURN_CODE 0
)
