set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usd)

pxr_cpp_bin(usdzip
    LIBRARIES
        tf
        sdf
        usd
        ar
        usdUtils
        usdValidation
)

pxr_register_test(testUsdZipInputFiles
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdzip flat.usdz src -l flat.txt"
    TESTENV testenv/testUsdZipInputFiles
    DIFF_COMPARE flat.txt
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdZipInputFiles2
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdzip -r recursive.usdz src -l recursive.txt"
    TESTENV testenv/testUsdZipInputFiles
    DIFF_COMPARE recursive.txt
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdZipInputFiles3
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdzip flat_inputs.usdz src/a.txt src/b.png"
    TESTENV testenv/testUsdZipInputFiles
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdZipInputFiles4
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdzip recursive_inputs.usdz src/a.txt src/b.png src/sub"
    TESTENV testenv/testUsdZipInputFiles
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdZipMissingInput
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdzip nonexistent.usdz nonexistent.usd"
    EXPECTED_RETURN_CODE 1
)

pxr_register_test(testUsdZipAsset
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdzip -a root.usd package.usdz -l contents.txt"
    TESTENV testenv/testUsdZipAsset
    DIFF_COMPARE contents.txt
    EXPECTED_RETURN_CODE 0
)
