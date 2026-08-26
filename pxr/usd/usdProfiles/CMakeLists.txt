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
        profilesDocUtils
        tokens

    PUBLIC_HEADERS
        api.h

    PYMODULE_CPPFILES
        wrapClaimsAPI.cpp
        wrapProfileRegistry.cpp
        wrapProfilesDocUtils.cpp
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

pxr_build_test(testUsdProfilesSchemaImplication
    LIBRARIES
        tf
        vt
        sdf
        usd
        usdProfiles
    CPPFILES
        testenv/testUsdProfilesSchemaImplication.cpp
)

pxr_register_test(testUsdProfilesSchemaImplication
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdProfilesSchemaImplication"
)

# Side-installed test plugin providing a minimal SdfFileFormat subclass plus
# its own seeded capability sub-graph. Lets us regress the file-format ->
# capability implication machinery without depending on any optional plugin
# (e.g. usdMtlx).
pxr_build_test_plugin(TestUsdProfilesPhonyFileFormat
    INSTALL_PREFIX UsdProfilesPlugins
    LIBRARIES
        sdf
        tf
    CPPFILES
        testenv/TestUsdProfilesPhonyFileFormat.cpp
)

pxr_build_test(testUsdProfilesFileFormatImplication
    LIBRARIES
        arch
        plug
        tf
        vt
        sdf
        usd
        usdProfiles
    CPPFILES
        testenv/testUsdProfilesFileFormatImplication.cpp
)

# This test relies on a plugin built specifically for this test.
# The build system currently doesn't support this for static library
# builds, so we only enable this test on shared library builds.
if (BUILD_SHARED_LIBS)
    pxr_register_test(testUsdProfilesFileFormatImplication
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdProfilesFileFormatImplication"
        TESTENV testenv/testUsdProfilesFileFormatImplication
    )
endif()

pxr_test_scripts(
    testenv/testUsdProfilesRegistryPy.py
    testenv/testUsdProfilesProfilesPy.py
    testenv/testUsdProfilesClaimsAPIPy.py
    testenv/testUsdProfilesSchemaImplicationPy.py
    testenv/testUsdProfilesFileFormatImplicationPy.py
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

pxr_register_test(testUsdProfilesSchemaImplicationPy
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdProfilesSchemaImplicationPy"
)

pxr_register_test(testUsdProfilesFileFormatImplicationPy
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdProfilesFileFormatImplicationPy"
    TESTENV testenv/testUsdProfilesFileFormatImplication
)
