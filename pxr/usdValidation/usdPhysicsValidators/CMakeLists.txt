set(PXR_PREFIX pxr/usdValidation)
set(PXR_PACKAGE usdPhysicsValidators)

pxr_library(usdPhysicsValidators
    LIBRARIES
        tf
        plug
        sdf
        usd
        usdGeom
        usdPhysics
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

pxr_test_scripts(
    testenv/testUsdPhysicsValidation.py
)

pxr_register_test(testUsdPhysicsValidation
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdPhysicsValidation"
    EXPECTED_RETURN_CODE 0
)

