set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usd)

pxr_cpp_bin(usdcat
        LIBRARIES
        tf
        sdf
        usd
        usdUtils
        )

pxr_register_test(testUsdCatOutToFile
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdcat input.usda --out output.usda"
    TESTENV testenv/testCatToFile
    DIFF_COMPARE output.usda
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdCatVariadicCatting
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdcat a.usd b.usd c.usd"
    TESTENV testenv/testVariadicCatting
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdCatMissingOrInvalidFiles1
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdcat hello.txt"
    TESTENV testenv/testMissingOrInvalidFiles
    EXPECTED_RETURN_CODE 1 
)

pxr_register_test(testUsdCatMissingOrInvalidFiles2
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdcat foo.usda"
    TESTENV testenv/testMissingOrInvalidFiles
    EXPECTED_RETURN_CODE 1 
)

pxr_register_test(testUsdCatMissingOrInvalidFiles3
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdcat --flattenLayerStack missingFile.usda"
    EXPECTED_RETURN_CODE 1
)

pxr_register_test(testUsdCatMask
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdcat --mask /InsideMask --flatten --skipSourceFileComment input.usda -o output.usda"
    TESTENV testenv/testUsdCatMask
    DIFF_COMPARE output.usda
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdCatMask2
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdcat --mask /InsideMask input.usda -o output.usda"
    EXPECTED_RETURN_CODE 1
)

pxr_register_test(testUsdCatLoadOnly
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdcat --loadOnly input.usda"
    TESTENV testenv/testLoadOnly
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdCatLayerMetadata
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdcat --layerMetadata input.usda -o output.usda"
    TESTENV testenv/testUsdCatLayerMetadata
    DIFF_COMPARE output.usda
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdCatFlattenLayerStack
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdcat --flattenLayerStack input.usda --out output.usda"
    TESTENV testenv/testUsdCatFlattenLayerStack
    DIFF_COMPARE output.usda
    EXPECTED_RETURN_CODE 0
)
