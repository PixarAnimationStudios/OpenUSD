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

# Run usdchecker tests which use the old UsdUtilsComplianceChecker python
# module, which is not available on non-python builds or in static builds.
# Hence these tests only run when python support is enabled or when building
# shared library.
if (PXR_ENABLE_PYTHON_SUPPORT AND BUILD_SHARED_LIBS)

pxr_install_test_dir(
    SRC testenv/testUsdChecker
    DEST testUsdChecker1
)

pxr_install_test_dir(
    SRC testenv/testUsdChecker
    DEST testUsdChecker2
)

pxr_install_test_dir(
    SRC testenv/testUsdChecker
    DEST testUsdChecker3
)

pxr_install_test_dir(
    SRC testenv/testUsdChecker
    DEST testUsdChecker4
)

pxr_register_test(testUsdChecker1
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdchecker clean/clean.usd"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdChecker2
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdchecker clean/clean_flat.usdc"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdChecker3
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdchecker clean/clean.usdz"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdChecker4
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdchecker clean/clean_flat.usdz"
    EXPECTED_RETURN_CODE 0
)

pxr_install_test_dir(
    SRC testenv/testUsdChecker
    DEST testUsdChecker5
)

pxr_install_test_dir(
    SRC testenv/testUsdChecker
    DEST testUsdChecker6
)

pxr_install_test_dir(
    SRC testenv/testUsdChecker
    DEST testUsdChecker7
)

pxr_install_test_dir(
    SRC testenv/testUsdChecker
    DEST testUsdChecker8
)

pxr_register_test(testUsdChecker5
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdchecker --arkit clean/clean_arkit.usdz"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdChecker6
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdchecker --arkit clean/clean_flat.usdc"
    EXPECTED_RETURN_CODE 1
)

# Adding a "rootPackageOnly" option to the command line will make the clean files
pxr_register_test(testUsdChecker7
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdchecker --arkit --rootPackageOnly clean/clean_flat.usdz"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdChecker8
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdchecker --arkit clean/clean_flat.usdz"
    EXPECTED_RETURN_CODE 1
)

pxr_install_test_dir(
    SRC testenv/testUsdChecker
    DEST testUsdChecker9
)

pxr_install_test_dir(
    SRC testenv/testUsdChecker
    DEST testUsdChecker10
)

pxr_install_test_dir(
    SRC testenv/testUsdChecker
    DEST testUsdChecker11
)

pxr_install_test_dir(
    SRC testenv/testUsdChecker
    DEST testUsdChecker12
)

pxr_register_test(testUsdChecker9
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdchecker bad/badUsdz.usdz"
    EXPECTED_RETURN_CODE 1
)

pxr_register_test(testUsdChecker10
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdchecker --arkit --dumpRules bad/variants.usdc -o variants_failedChecks.txt"
    DIFF_COMPARE variants_failedChecks.txt
    EXPECTED_RETURN_CODE 1
)

pxr_register_test(testUsdChecker11
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdchecker bad/brokenRef.usd"
    EXPECTED_RETURN_CODE 1
)

pxr_register_test(testUsdChecker12
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdchecker --arkit bad/badShaderUnsupportedTexture.usdc"
    EXPECTED_RETURN_CODE 1
)

pxr_install_test_dir(
    SRC testenv/testUsdChecker
    DEST testUsdChecker13
)

pxr_register_test(testUsdChecker13
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdchecker clean/cleanNormalMapReader.usda"
    EXPECTED_RETURN_CODE 0
)

endif()

# Some of the tests trigger various plugin mechanism, including ndr, so
# disabling tests for static builds.
if (NOT BUILD_SHARED_LIBS) 
    return()
endif()

# Following are all usdchecker tests using the new --useNewValidationFramework
# flag. These tests are expected to pass with the new validation framework.
pxr_install_test_dir(
    SRC testenv/testUsdChecker
    DEST testUsdChecker14
)

pxr_install_test_dir(
    SRC testenv/testUsdChecker
    DEST testUsdChecker15
)

pxr_install_test_dir(
    SRC testenv/testUsdChecker
    DEST testUsdChecker16
)

pxr_install_test_dir(
    SRC testenv/testUsdChecker
    DEST testUsdChecker17
)

pxr_register_test(testUsdChecker14
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdchecker --useNewValidationFramework clean/clean.usd"
    EXPECTED_RETURN_CODE 0
)


pxr_register_test(testUsdChecker15
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdchecker --useNewValidationFramework clean/clean_flat.usdc"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdChecker16
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdchecker --useNewValidationFramework clean/clean.usdz"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdChecker17
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdchecker --useNewValidationFramework clean/clean_flat.usdz"
    EXPECTED_RETURN_CODE 0
)


pxr_install_test_dir(
    SRC testenv/testUsdChecker
    DEST testUsdChecker18
)

pxr_install_test_dir(
    SRC testenv/testUsdChecker
    DEST testUsdChecker19
)

pxr_install_test_dir(
    SRC testenv/testUsdChecker
    DEST testUsdChecker20
)

pxr_install_test_dir(
    SRC testenv/testUsdChecker
    DEST testUsdChecker21
)

pxr_register_test(testUsdChecker18
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdchecker --useNewValidationFramework --arkit clean/clean_arkit.usdz"
    EXPECTED_RETURN_CODE 0
)

#Adding the arkit option makes the clean files fail some checks, test for that.
#But validation rules are not yet ported for arkit / usdz validator, hence
#following are disabled.

#pxr_register_test(testUsdChecker19
#    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdchecker --useNewValidationFramework --arkit clean/clean_flat.usdc"
#    EXPECTED_RETURN_CODE 1
#)

# "rootPackageOnly" won't report validation errors within the subpackages, so it should pass
pxr_register_test(testUsdChecker20
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdchecker --useNewValidationFramework --arkit --rootPackageOnly bad/sceneWithInvalidPackage.usda"
    EXPECTED_RETURN_CODE 0
)

# Removing rootPackageOnly, should fail as the subpackage has invalid content
pxr_register_test(testUsdChecker21
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdchecker --useNewValidationFramework --arkit bad/sceneWithInvalidPackage.usda"
    EXPECTED_RETURN_CODE 1
)

pxr_install_test_dir(
    SRC testenv/testUsdChecker
    DEST testUsdChecker22
)

pxr_install_test_dir(
    SRC testenv/testUsdChecker
    DEST testUsdChecker23
)

pxr_install_test_dir(
    SRC testenv/testUsdChecker
    DEST testUsdChecker24
)

pxr_install_test_dir(
    SRC testenv/testUsdChecker
    DEST testUsdChecker25
)

pxr_install_test_dir(
    SRC testenv/testUsdChecker
    DEST testUsdChecker26
)

pxr_install_test_dir(
    SRC testenv/testUsdChecker
    DEST testUsdChecker27
)

pxr_install_test_dir(
    SRC testenv/testUsdChecker
    DEST testUsdChecker28
)

pxr_install_test_dir(
    SRC testenv/testUsdChecker
    DEST testUsdChecker29
)

pxr_install_test_dir(
    SRC testenv/testUsdChecker
    DEST testUsdChecker30
)

pxr_install_test_dir(
    SRC testenv/testUsdChecker
    DEST testUsdChecker31
)

pxr_install_test_dir(
    SRC testenv/testUsdChecker
    DEST testUsdChecker32
)

pxr_install_test_dir(
    SRC testenv/testUsdChecker
    DEST testUsdChecker33
)

pxr_install_test_dir(
    SRC testenv/testUsdChecker
    DEST testUsdChecker34
)

pxr_register_test(testUsdChecker22
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdchecker --useNewValidationFramework bad/badUsdz.usdz"
    EXPECTED_RETURN_CODE 1
)

#The bad/variants.usdc file uses Point instancer and no point instance validator
#is not ported to new validation framework yet, hence using a different variant
#asset.

# Simple variant file, showing all combination of variants being validated and
# failing
pxr_register_test(testUsdChecker23
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdchecker --useNewValidationFramework --arkit --dumpRules bad/badVariants.usdc"
    EXPECTED_RETURN_CODE 1
)

# Complex variants, with default variant being clean but still one of the
# combination of variant failing.
pxr_register_test(testUsdChecker24
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdchecker --useNewValidationFramework bad/badVariants2.usda"
    EXPECTED_RETURN_CODE 1
)

# Explicitly set broken variant combination, should fail
pxr_register_test(testUsdChecker25
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdchecker --useNewValidationFramework bad/badVariants2.usda --variants testSet:test2,testSet2:test2C"
    EXPECTED_RETURN_CODE 1
)

# Explicitly set clean variant combination, should pass
pxr_register_test(testUsdChecker26
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdchecker --useNewValidationFramework bad/badVariants2.usda --variants testSet:test1,testSet2:test2A"
    EXPECTED_RETURN_CODE 0
)

# Explicitly set variant combination which doesn't exist (test2C doesn't exist
# in testSet:test1 variant, should pass
pxr_register_test(testUsdChecker27
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdchecker --useNewValidationFramework bad/badVariants2.usda --variants testSet:test1,testSet2:test2C"
    EXPECTED_RETURN_CODE 0
)

# Explicitly set variantSets, one combination of testSet variantSet will fail
pxr_register_test(testUsdChecker28
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdchecker --useNewValidationFramework bad/badVariants2.usda --variantSets testSet"
    EXPECTED_RETURN_CODE 1
)


# Complex variant example with variant on a prim changing its variantSet
# This test should fail as the variantSet set1 has a variant ref2, which brings in more variantSets set2 and set3, and one of these defaults set3:D fails validation
pxr_register_test(testUsdChecker29
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdchecker --useNewValidationFramework bad/badVariants3.usda"
    EXPECTED_RETURN_CODE 1
)

# When running variantSets set1 explicitly, it traverses the set1:ref2 which again brings in a default set3:D which fails validation
pxr_register_test(testUsdChecker30
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdchecker --useNewValidationFramework bad/badVariants3.usda --variantSets set1"
    EXPECTED_RETURN_CODE 1
)

# variantSets set2, set3 do not exist by default, as set1:ref1 default on the /World prim, doesn't bring in any new variant set via a reference and hence doesn't fail validation.
pxr_register_test(testUsdChecker31
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdchecker --useNewValidationFramework bad/badVariants3.usda --variantSets set2"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdChecker32
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdchecker --useNewValidationFramework bad/badVariants3.usda --variantSets set3"
    EXPECTED_RETURN_CODE 0
)

# Running a specific variant combination which fails validation
pxr_register_test(testUsdChecker33
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdchecker --useNewValidationFramework bad/badVariants3.usda --variants set1:ref2,set2:A,set3:D"
    EXPECTED_RETURN_CODE 1
)

# Running a specific variant with variants from a specific variantSet (one of this combination fails validation, set1:ref2 set3:D)
pxr_register_test(testUsdChecker34
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdchecker --useNewValidationFramework bad/badVariants3.usda --variants set1:ref2 --variantSets set3"
    EXPECTED_RETURN_CODE 1
)

pxr_install_test_dir(
    SRC testenv/testUsdChecker
    DEST testUsdChecker35
)

pxr_install_test_dir(
    SRC testenv/testUsdChecker
    DEST testUsdChecker36
)

pxr_register_test(testUsdChecker35
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdchecker --useNewValidationFramework bad/brokenRef.usd"
    EXPECTED_RETURN_CODE 1
)

pxr_register_test(testUsdChecker36
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdchecker --useNewValidationFramework --arkit bad/badShaderUnsupportedTexture.usdc"
    EXPECTED_RETURN_CODE 1
)

pxr_install_test_dir(
    SRC testenv/testUsdChecker
    DEST testUsdChecker37
)

pxr_register_test(testUsdChecker37
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdchecker --useNewValidationFramework clean/cleanNormalMapReader.usda"
    EXPECTED_RETURN_CODE 0
)
