set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usd)

pxr_python_bin(usdtree
    DEPENDENCIES
        ar
        sdf
        usd
)

pxr_install_test_dir(
    SRC testenv/testUsdTree
    DEST testUsdTree
)

pxr_register_test(testUsdTree
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdtree input.usda"
    STDOUT_REDIRECT output.txt
    DIFF_COMPARE output.txt
    EXPECTED_RETURN_CODE 0
)

pxr_install_test_dir(
    SRC testenv/testUsdTreePayload
    DEST testUsdTreeLoaded
)

pxr_register_test(testUsdTreeLoaded
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdtree input.usda"
    STDOUT_REDIRECT output.txt
    DIFF_COMPARE output.txt
    EXPECTED_RETURN_CODE 0
)

pxr_install_test_dir(
    SRC testenv/testUsdTreePayload
    DEST testUsdTreeUnloaded
)

pxr_register_test(testUsdTreeUnloaded
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdtree --unloaded input.usda"
    STDOUT_REDIRECT output_unloaded.txt
    DIFF_COMPARE output_unloaded.txt
    EXPECTED_RETURN_CODE 0
)


pxr_install_test_dir(
    SRC testenv/testUsdTree
    DEST testUsdTreeAttributes
)

pxr_register_test(testUsdTreeAttributes
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdtree --attributes --metadata --types input.usda"
    STDOUT_REDIRECT output_attributes.txt
    DIFF_COMPARE output_attributes.txt
    EXPECTED_RETURN_CODE 0
)

