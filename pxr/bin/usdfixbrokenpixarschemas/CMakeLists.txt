set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usd)

pxr_python_bin(usdfixbrokenpixarschemas
    DEPENDENCIES
        sdf
        usd
        usdUtils
        usdValidation
)

# XXX:
# Temporarily disable this test on Windows; Need to investigate more on
# appropriate POST_COMMAND for result usdz diffing on windows.
if (NOT WIN32)
pxr_register_test(testUsdzMaterialBindingAPI
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdfixbrokenpixarschemas nested_anchored_refs_sub.usdz --backup backup.usdz"
    TESTENV testenv/testUsdzMaterialBindingAPI
    POST_COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usddiff --noeffect --brief nested_anchored_refs_sub.usdz baseline/nested_anchored_refs_sub.usdz"
    POST_COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usddiff --noeffect --brief backup.usdz baseline/backup.usdz"
    EXPECTED_RETURN_CODE 0
    CLEAN_OUTPUT
)
endif()

pxr_register_test(testInvocationErrorInvalidFile
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdfixbrokenpixarschemas invalid.usda"
    EXPECTED_RETURN_CODE 1
)

pxr_register_test(testInvocationErrorInvalidExtension
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdfixbrokenpixarschemas file.txt"
    TESTENV testenv/testInvocationErrors
    EXPECTED_RETURN_CODE 1
)

pxr_register_test(testMultipleFixer
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdfixbrokenpixarschemas test.usda --backup backup.usda"
    TESTENV testenv/testMultipleFixer
    POST_COMMAND "${CMAKE_COMMAND} -E copy test.usda test_Yup.usda"
    DIFF_COMPARE test_Yup.usda
    DIFF_COMPARE backup.usda
    EXPECTED_RETURN_CODE 0
    CLEAN_OUTPUT
)

pxr_register_test(testVariantMaterialBindingAPI
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdfixbrokenpixarschemas test.usda --backup backup.usda"
    TESTENV testenv/testVariantMaterialBindingAPI
    DIFF_COMPARE test.usda
    DIFF_COMPARE backup.usda
    EXPECTED_RETURN_CODE 0
    CLEAN_OUTPUT
)

if (PXR_BUILD_PRMAN_PLUGIN)
pxr_register_test(testRenderSettingsTerminalsAPI
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdfixbrokenpixarschemas test.usda --backup backup.usda"
    TESTENV testenv/testRenderSettingsTerminalsAPI
    DIFF_COMPARE test.usda
    DIFF_COMPARE backup.usda
    EXPECTED_RETURN_CODE 0
    CLEAN_OUTPUT
)
endif()
