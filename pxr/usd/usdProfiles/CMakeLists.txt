set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usdProfiles)

pxr_library(usdProfiles
    INCLUDE_SCHEMA_FILES

    LIBRARIES
        arch
        tf
        plug
        vt
        js
        sdf
        usd

    PUBLIC_CLASSES
        claimsAPI
        profileRegistry
        tokens

    PUBLIC_HEADERS
        api.h

    PYMODULE_CPPFILES
        wrapClaimsAPI.cpp
        wrapProfileRegistry.cpp
        wrapTokens.cpp

    PYMODULE_FILES
        __init__.py
)

pxr_build_test(testUsdProfilesRegistry
    LIBRARIES
        tf
        js
        vt
        usdProfiles
    CPPFILES
        testenv/testUsdProfilesRegistry.cpp
)

pxr_register_test(testUsdProfilesRegistry
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdProfilesRegistry"
    TESTENV testenv/testUsdProfilesRegistry
)

pxr_build_test(testUsdProfilesProfiles
    LIBRARIES
        tf
        js
        vt
        sdf
        usd
        usdProfiles
    CPPFILES
        testenv/testUsdProfilesProfiles.cpp
)

pxr_register_test(testUsdProfilesProfiles
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdProfilesProfiles"
    TESTENV testenv/testUsdProfilesRegistry
)

pxr_build_test(testUsdProfilesClaimsAPI
    LIBRARIES
        tf
        vt
        sdf
        usd
        usdProfiles
    CPPFILES
        testenv/testUsdProfilesClaimsAPI.cpp
)

pxr_register_test(testUsdProfilesClaimsAPI
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdProfilesClaimsAPI"
)

pxr_test_scripts(
    testenv/testUsdProfilesRegistryPy.py
    testenv/testUsdProfilesProfilesPy.py
    testenv/testUsdProfilesClaimsAPIPy.py
)

pxr_register_test(testUsdProfilesRegistryPy
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdProfilesRegistryPy"
    TESTENV testenv/testUsdProfilesRegistry
)

pxr_register_test(testUsdProfilesProfilesPy
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdProfilesProfilesPy"
    TESTENV testenv/testUsdProfilesRegistry
)

pxr_register_test(testUsdProfilesClaimsAPIPy
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdProfilesClaimsAPIPy"
)
