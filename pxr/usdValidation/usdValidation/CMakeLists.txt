set(PXR_PREFIX pxr/usdValidation)
set(PXR_PACKAGE usdValidation)

pxr_library(usdValidation
    LIBRARIES
        sdf
        plug
        tf
        gf
        usd
        work
        ${TBB_tbb_LIBRARY}

    INCLUDE_DIRS
        ${TBB_INCLUDE_DIRS}

    PUBLIC_CLASSES
        context 
        error
        registry
        timeRange
        validator
        validatorTokens

    PUBLIC_HEADERS
        api.h

    CPPFILES
        coreValidators.cpp

    PYMODULE_CPPFILES
        module.cpp
        wrapContext.cpp
        wrapError.cpp
        wrapRegistry.cpp
        wrapTimeRange.cpp
        wrapValidator.cpp

    PYMODULE_FILES
        __init__.py

    RESOURCE_FILES
        plugInfo.json
)

pxr_test_scripts(
    testenv/testUsdValidationContextPy.py
    testenv/testUsdValidationError.py
    testenv/testUsdValidationRegistryPy.py
    testenv/testUsdValidatorMetadata.py
)

pxr_build_test_shared_lib(TestUsdValidationContextValidators
    INSTALL_PREFIX UsdValidationPlugins
    LIBRARIES
        tf
        usd
        usdValidation

    CPPFILES
        testenv/testUsdValidationContextValidators.cpp
)

pxr_build_test_shared_lib(TestUsdValidationRegistryPy
    INSTALL_PREFIX UsdValidationPlugins
    LIBRARIES
        tf
        usd
        usdValidation

    CPPFILES
        testenv/testUsdValidationRegistryPy.cpp
)

pxr_build_test(testUsdCoreValidators
    LIBRARIES
        usd
        usdValidation

    CPPFILES
        testenv/testUsdCoreValidators.cpp
)

pxr_build_test(testUsdValidator
    LIBRARIES
        tf
        usd
        usdValidation

    CPPFILES
        testenv/testUsdValidator.cpp
)

pxr_build_test(testUsdValidationContext
    LIBRARIES
        tf
        usd
        usdValidation

    CPPFILES
        testenv/testUsdValidationContext.cpp
)

pxr_build_test(testUsdValidationRegistry
    LIBRARIES
        tf
        usd
        usdValidation

    CPPFILES
        testenv/testUsdValidationRegistry.cpp
)

pxr_install_test_dir(
    SRC testenv/testUsdValidationContext
    DEST testUsdValidationContext
)

pxr_install_test_dir(
    SRC testenv/testUsdValidationContext
    DEST testUsdValidationContextPy
)

pxr_install_test_dir(
    SRC testenv/testUsdValidationRegistry
    DEST testUsdValidationRegistry
)

pxr_register_test(testUsdCoreValidators
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdCoreValidators"
        EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdValidator
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdValidator"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdValidationRegistry
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdValidationRegistry"
    EXPECTED_RETURN_CODE 0
)

#The following tests rely on plugins built specifically for them.
#The build system currently doesn't support test-specific plugins
#for static library builds, so we only enable these tests for
#shared library builds.
if (BUILD_SHARED_LIBS)
    pxr_register_test(testUsdValidationContext
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdValidationContext"
        EXPECTED_RETURN_CODE 0
    )
endif()

pxr_register_test(testUsdValidatorMetadata
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdValidatorMetadata"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdValidationError
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdValidationError"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdValidationContextPy
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdValidationContextPy"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdValidationRegistryPy
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdValidationRegistryPy"
    EXPECTED_RETURN_CODE 0
)
