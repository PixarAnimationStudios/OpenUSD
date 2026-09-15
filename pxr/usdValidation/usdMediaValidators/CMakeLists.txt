set(PXR_PREFIX pxr/usdValidation)
set(PXR_PACKAGE usdMediaValidators)

pxr_library(usdMediaValidators
    LIBRARIES
        tf
        sdf
        usd
        usdMedia
        usdValidation

    PUBLIC_CLASSES
        validatorTokens

    PUBLIC_HEADERS
        api.h

    CPPFILES
        validators.cpp

    RESOURCE_FILES
        plugInfo.json

    DISABLE_PRECOMPILED_HEADERS
)

pxr_build_test(testUsdMediaValidators
    LIBRARIES
        tf
        sdf
        usd
        usdMedia
        usdValidation
        usdMediaValidators
    CPPFILES
        testenv/testUsdMediaValidators.cpp
)

# Tests that rely on plugins aren't currently supported in static builds.
if (BUILD_SHARED_LIBS)
    pxr_register_test(testUsdMediaValidators
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdMediaValidators"
        EXPECTED_RETURN_CODE 0
    )
endif()
