set(PXR_PREFIX pxr/usdValidation)
set(PXR_PACKAGE usdLodValidators)

pxr_library(usdLodValidators
    LIBRARIES
        tf
        plug
        vt
        sdf
        usd
        usdLod
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

pxr_build_test(testUsdLodValidators
    LIBRARIES
        tf
        sdf
        usd
        usdLod
        usdValidation
        usdLodValidators
    CPPFILES
        testenv/testUsdLodValidators.cpp
)

# Tests that rely on plugins aren't currently supported in static builds.
if (BUILD_SHARED_LIBS)
    pxr_register_test(testUsdLodValidators
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdLodValidators"
        EXPECTED_RETURN_CODE 0
    )
endif()
