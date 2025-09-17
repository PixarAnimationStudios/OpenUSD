set(PXR_PREFIX pxr/usdValidation)
set(PXR_PACKAGE usdGeomValidators)

pxr_library(usdGeomValidators
    LIBRARIES
        tf
        plug
        sdf
        usd
        usdGeom
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

pxr_build_test(testUsdGeomValidators
    LIBRARIES
        tf
        usd
        usdValidation
        usdGeom
        usdGeomValidators
    CPPFILES
        testenv/testUsdGeomValidators.cpp
)

pxr_register_test(testUsdGeomValidators
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdGeomValidators"
        EXPECTED_RETURN_CODE 0
)

