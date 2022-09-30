set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usd)

pxr_python_bin(usdfixbrokenpixarschemas
    DEPENDENCIES
        sdf
        usd
)

pxr_install_test_dir(
    SRC testenv/testMultipleFixer
    DEST testMultipleFixer
)

pxr_install_test_dir(
    SRC testenv/testVariantMaterialBindingAPI
    DEST testVariantMaterialBindingAPI
)
# XXX:
# Temporarily disable this test on Windows; Some extracted usdz files are not
# getting removed because of access denied error and the test fails on windows.
if (NOT WIN32)
pxr_install_test_dir(
    SRC testenv/testUsdzMaterialBindingAPI
    DEST testUsdzMaterialBindingAPI
)

pxr_register_test(testUsdzMaterialBindingAPI
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdfixbrokenpixarschemas nested_anchored_refs_sub.usdz --backup backup.usdz"
    POST_COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usddiff --noeffect --brief nested_anchored_refs_sub.usdz baseline/nested_anchored_refs_sub.usdz"
    POST_COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usddiff --noeffect --brief backup.usdz baseline/backup.usdz"
    EXPECTED_RETURN_CODE 0
    CLEAN_OUTPUT
)
endif()

pxr_register_test(testMultipleFixer
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdfixbrokenpixarschemas test.usda --backup backup.usda"
    POST_COMMAND "${CMAKE_COMMAND} -E copy test.usda test_Yup.usda"
    DIFF_COMPARE test_Yup.usda
    DIFF_COMPARE backup.usda
    EXPECTED_RETURN_CODE 0
    CLEAN_OUTPUT
)

pxr_register_test(testVariantMaterialBindingAPI
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdfixbrokenpixarschemas test.usda --backup backup.usda"
    DIFF_COMPARE test.usda
    DIFF_COMPARE backup.usda
    EXPECTED_RETURN_CODE 0
    CLEAN_OUTPUT
)
