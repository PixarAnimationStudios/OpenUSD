set(PXR_PREFIX pxr/usdValidation)
set(PXR_PACKAGE usdchecker)

pxr_cpp_bin(usdchecker
    LIBRARIES
        arch
        tf
        sdf
        usd
        usdValidation
        usdUtils
        usdUtilsValidators
)

# Some of the tests trigger various plugin mechanism, including ndr, so
# disabling tests for static builds.
if (NOT BUILD_SHARED_LIBS) 
    return()
endif()

# Following are all usdchecker tests using the new Validation Framework.
# These tests are expected to pass with the new validation framework.
pxr_register_test(testUsdChecker1
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdchecker clean/clean.usd"
    TESTENV testenv/testUsdChecker
    EXPECTED_RETURN_CODE 0
)


pxr_register_test(testUsdChecker2
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdchecker clean/clean_flat.usdc"
    TESTENV testenv/testUsdChecker
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdChecker3
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdchecker clean/clean.usdz"
    TESTENV testenv/testUsdChecker
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdChecker4
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdchecker clean/clean_flat.usdz"
    TESTENV testenv/testUsdChecker
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdChecker5
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdchecker clean/clean_arkit.usdz"
    TESTENV testenv/testUsdChecker
    EXPECTED_RETURN_CODE 0
)

# "rootPackageOnly" won't report validation errors within the subpackages, so it should pass
pxr_register_test(testUsdChecker6
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdchecker --rootPackageOnly bad/sceneWithInvalidPackage.usda"
    TESTENV testenv/testUsdChecker
    EXPECTED_RETURN_CODE 0
)

# Removing rootPackageOnly, should fail as the subpackage has invalid content
pxr_register_test(testUsdChecker7
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdchecker bad/sceneWithInvalidPackage.usda"
    TESTENV testenv/testUsdChecker
    EXPECTED_RETURN_CODE 1
)

pxr_register_test(testUsdChecker8
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdchecker bad/badUsdz.usdz"
    TESTENV testenv/testUsdChecker
    EXPECTED_RETURN_CODE 1
)

# Simple variant file, showing all combination of variants being validated and
# failing
pxr_register_test(testUsdChecker9
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdchecker --dumpRules bad/badVariants.usdc"
    TESTENV testenv/testUsdChecker
    EXPECTED_RETURN_CODE 1
)

# Complex variants, with default variant being clean but still one of the
# combination of variant failing.
pxr_register_test(testUsdChecker10
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdchecker bad/badVariants2.usda"
    TESTENV testenv/testUsdChecker
    EXPECTED_RETURN_CODE 1
)

# Explicitly set broken variant combination, should fail
pxr_register_test(testUsdChecker11
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdchecker bad/badVariants2.usda --variants testSet:test2,testSet2:test2C"
    TESTENV testenv/testUsdChecker
    EXPECTED_RETURN_CODE 1
)

# Explicitly set clean variant combination, should pass
pxr_register_test(testUsdChecker12
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdchecker bad/badVariants2.usda --variants testSet:test1,testSet2:test2A"
    TESTENV testenv/testUsdChecker
    EXPECTED_RETURN_CODE 0
)

# Explicitly set variant combination which doesn't exist (test2C doesn't exist
# in testSet:test1 variant, should pass
pxr_register_test(testUsdChecker13
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdchecker bad/badVariants2.usda --variants testSet:test1,testSet2:test2C"
    TESTENV testenv/testUsdChecker
    EXPECTED_RETURN_CODE 0
)

# Explicitly set variantSets, one combination of testSet variantSet will fail
pxr_register_test(testUsdChecker14
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdchecker bad/badVariants2.usda --variantSets testSet"
    TESTENV testenv/testUsdChecker
    EXPECTED_RETURN_CODE 1
)

# Complex variant example with variant on a prim changing its variantSet
# This test should fail as the variantSet set1 has a variant ref2, which brings in more variantSets set2 and set3, and one of these defaults set3:D fails validation
pxr_register_test(testUsdChecker15
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdchecker bad/badVariants3.usda"
    TESTENV testenv/testUsdChecker
    EXPECTED_RETURN_CODE 1
)

# When running variantSets set1 explicitly, it traverses the set1:ref2 which again brings in a default set3:D which fails validation
pxr_register_test(testUsdChecker16
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdchecker bad/badVariants3.usda --variantSets set1"
    TESTENV testenv/testUsdChecker
    EXPECTED_RETURN_CODE 1
)

# variantSets set2, set3 do not exist by default, as set1:ref1 default on the /World prim, doesn't bring in any new variant set via a reference and hence doesn't fail validation.
pxr_register_test(testUsdChecker17
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdchecker bad/badVariants3.usda --variantSets set2"
    TESTENV testenv/testUsdChecker
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdChecker18
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdchecker bad/badVariants3.usda --variantSets set3"
    TESTENV testenv/testUsdChecker
    EXPECTED_RETURN_CODE 0
)

# Running a specific variant combination which fails validation
pxr_register_test(testUsdChecker19
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdchecker bad/badVariants3.usda --variants set1:ref2,set2:A,set3:D"
    TESTENV testenv/testUsdChecker
    EXPECTED_RETURN_CODE 1
)

# Running a specific variant with variants from a specific variantSet (one of this combination fails validation, set1:ref2 set3:D)
pxr_register_test(testUsdChecker20
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdchecker bad/badVariants3.usda --variants set1:ref2 --variantSets set3"
    TESTENV testenv/testUsdChecker
    EXPECTED_RETURN_CODE 1
)

pxr_register_test(testUsdChecker21
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdchecker bad/brokenRef.usd"
    TESTENV testenv/testUsdChecker
    EXPECTED_RETURN_CODE 1
)

pxr_register_test(testUsdChecker22
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdchecker bad/badShaderUnsupportedTexture.usdc"
    TESTENV testenv/testUsdChecker
    EXPECTED_RETURN_CODE 1
)

pxr_register_test(testUsdChecker23
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdchecker clean/cleanNormalMapReader.usda"
    TESTENV testenv/testUsdChecker
    EXPECTED_RETURN_CODE 0
)
