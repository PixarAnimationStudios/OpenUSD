set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usd)

pxr_python_bin(usdcat
    DEPENDENCIES
        ar
        sdf
        usd
)

pxr_install_test_dir(
    SRC testenv/testCatToFile
    DEST testUsdCatOutToFile
)

pxr_register_test(testUsdCatOutToFile
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdcat input.usda --out output.usda"
    DIFF_COMPARE output.usda
    EXPECTED_RETURN_CODE 0
)

pxr_install_test_dir(
    SRC testenv/testVariadicCatting
    DEST testUsdCatVariadicCatting
)

pxr_register_test(testUsdCatVariadicCatting
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdcat a.usd b.usd c.usd"
    EXPECTED_RETURN_CODE 0
)

pxr_install_test_dir(
    SRC testenv/testMissingOrInvalidFiles
    DEST testUsdCatMissingOrInvalidFiles1
)

pxr_register_test(testUsdCatMissingOrInvalidFiles1
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdcat hello.txt"
    EXPECTED_RETURN_CODE 1 
)

pxr_install_test_dir(
    SRC testenv/testMissingOrInvalidFiles
    DEST testUsdCatMissingOrInvalidFiles2
)

pxr_register_test(testUsdCatMissingOrInvalidFiles2
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdcat foo.usda"
    EXPECTED_RETURN_CODE 1 
)

pxr_install_test_dir(
    SRC testenv/testUsdCatMask
    DEST testUsdCatMask
)

pxr_register_test(testUsdCatMask
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdcat --mask /InsideMask --flatten --skipSourceFileComment input.usda -o output.usda"
    DIFF_COMPARE output.usda
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdCatMask2
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdcat --mask /InsideMask input.usda -o output.usda"
    EXPECTED_RETURN_CODE 1
)

pxr_install_test_dir(
    SRC testenv/testLoadOnly
    DEST testUsdCatLoadOnly
)

pxr_register_test(testUsdCatLoadOnly
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdcat --loadOnly input.usda"
    EXPECTED_RETURN_CODE 0
)
