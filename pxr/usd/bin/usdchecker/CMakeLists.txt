set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usd)

pxr_python_bin(usdchecker
    DEPENDENCIES
        usd
        usdGeom
)

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
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdchecker clean/clean.usd"
    EXPECTED_RETURN_CODE 0
)


pxr_register_test(testUsdChecker2
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdchecker clean/clean_flat.usdc"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdChecker3
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdchecker clean/clean.usdz"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdChecker4
    PYTHON
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
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdchecker --arkit clean/clean_arkit.usdz"
    EXPECTED_RETURN_CODE 0
)


pxr_register_test(testUsdChecker6
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdchecker --arkit clean/clean_flat.usdc"
    EXPECTED_RETURN_CODE 1
)

pxr_register_test(testUsdChecker7
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdchecker --arkit --rootPackageOnly clean/clean_flat.usdz"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdChecker8
    PYTHON
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
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdchecker bad/badUsdz.usdz"
    EXPECTED_RETURN_CODE 1
)


pxr_register_test(testUsdChecker10
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdchecker --arkit --dumpRules bad/variants.usdc -o variants_failedChecks.txt"
    DIFF_COMPARE variants_failedChecks.txt
    EXPECTED_RETURN_CODE 1
)

pxr_register_test(testUsdChecker11
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdchecker bad/brokenRef.usd"
    EXPECTED_RETURN_CODE 1
)

pxr_register_test(testUsdChecker12
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdchecker --arkit bad/badShaderUnsupportedTexture.usd"
    EXPECTED_RETURN_CODE 1
)

