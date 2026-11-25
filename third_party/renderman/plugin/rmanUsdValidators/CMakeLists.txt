set(PXR_PREFIX "")
set(PXR_PACKAGE rmanUsdValidators)

pxr_plugin(rmanUsdValidators
    LIBRARIES
        arch
        tf
        plug
        sdf
        usd
        usdGeom
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

if (NOT TARGET ${PXR_PACKAGE})
    return()
endif()

target_include_directories(${PXR_PACKAGE}
    PRIVATE
    "${PROJECT_SOURCE_DIR}/third_party/renderman/plugin"
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
