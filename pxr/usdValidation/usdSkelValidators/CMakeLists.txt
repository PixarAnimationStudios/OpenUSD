set(PXR_PREFIX pxr/usdValidation)
set(PXR_PACKAGE usdSkelValidators)

pxr_library(usdSkelValidators
    LIBRARIES
        tf
        plug
        sdf
        usd
        usdSkel
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

pxr_build_test(testUsdSkelValidators
    LIBRARIES
        tf
        usd
        usdValidation
        usdSkel
        usdSkelValidators
    CPPFILES
        testenv/testUsdSkelValidators.cpp
)

pxr_register_test(testUsdSkelValidators
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdSkelValidators"
    EXPECTED_RETURN_CODE 0
)

