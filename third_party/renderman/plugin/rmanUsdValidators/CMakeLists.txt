set(PXR_PREFIX "")
set(PXR_PACKAGE rmanUsdValidators)

pxr_plugin(rmanUsdValidators
    LIBRARIES
        tf
        plug
        sdf
        usd
        usdRender
        usdRi
        usdShade
        usdValidation

    PRIVATE_CLASSES
        validatorTokens

    CPPFILES
        validators.cpp

    RESOURCE_FILES
        plugInfo.json
)

pxr_build_test(testRmanUsdValidators
    LIBRARIES
        tf
        usd
        usdRender
        usdRi
        usdShade
        usdValidation
    CPPFILES
        testenv/testRmanUsdValidators.cpp
)

# Need to set RMANTREE and RMAN_SHADERPATH environment variable when running
# tests
get_filename_component(RMANTREE_PATH "${RENDERMAN_INCLUDE_DIR}/.." ABSOLUTE)

pxr_register_test(testRmanUsdValidators
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testRmanUsdValidators"
    EXPECTED_RETURN_CODE 0
    ENV
        RMANTREE=${RMANTREE_PATH}
)
