set(PXR_PREFIX pxr/usdValidation)
set(PXR_PACKAGE usdLuxValidators)

pxr_library(usdLuxValidators
    LIBRARIES
        tf
        plug
        sdf
        usd
        sdr
        usdShade
        usdLux
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

pxr_build_test(testUsdLuxValidators
    LIBRARIES
        tf
        sdr
        usd
        usdValidation
        usdLux
        usdLuxValidators
    CPPFILES
        testenv/testUsdLuxValidators.cpp
)

# Tests that rely on plugins aren't currently supported in static builds.
if (BUILD_SHARED_LIBS)
    pxr_register_test(testUsdLuxValidators
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdLuxValidators"
        EXPECTED_RETURN_CODE 0
    )
endif()
