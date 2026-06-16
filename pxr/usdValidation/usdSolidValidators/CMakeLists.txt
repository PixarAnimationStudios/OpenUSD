set(PXR_PREFIX pxr/usdValidation)
set(PXR_PACKAGE usdSolidValidators)

pxr_library(usdSolidValidators
    LIBRARIES
        tf
        plug
        sdf
        gf
        usd
        usdGeom
        usdSolid
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

pxr_build_test(testUsdSolidValidators
    LIBRARIES
        tf
        usd
        usdValidation
        usdSolid
        usdSolidValidators
    CPPFILES
        testenv/testUsdSolidValidators.cpp
)

pxr_register_test(testUsdSolidValidators
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdSolidValidators"
        EXPECTED_RETURN_CODE 0
)
