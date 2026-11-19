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

pxr_register_test(testUsdCatUnrecognizedExtension2
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdcat input.usda --out output.tmp --usdFormat usda"
    TESTENV testenv/testUsdCatUnrecognizedExtension
    DIFF_COMPARE output.tmp
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdCatUnrecognizedExtension4
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdcat input.usda --out unformatted"
    TESTENV testenv/testUsdCatUnrecognizedExtension
    EXPECTED_RETURN_CODE 1
)

pxr_register_test(testUsdCatUnrecognizedExtension5
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdcat input.usda --out output.usda --usdFormat usdc"
    TESTENV testenv/testUsdCatUnrecognizedExtension
    EXPECTED_RETURN_CODE 1
)

pxr_register_test(testUsdCatKeepExistingOnError
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdcat input.usda --out existing.usdz"
    TESTENV testenv/testUsdCatKeepExistingOnError
    FILES_EXIST existing.usdz
    FILES_DONT_EXIST existing.usdz.quarantine
    DIFF_COMPARE existing.usdz
    EXPECTED_RETURN_CODE 1
)

# Look up programs needed for checking the format of extensionless output.
find_program(HEADBIN
    NAMES head
)

if (NOT EXISTS ${HEADBIN})
    message(STATUS "Skipping usdcat unrecognized extension format tests, could not find head command.")
else()
    pxr_register_test(testUsdCatUnrecognizedExtension1
        COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdcat input.usda --out output --usdFormat usdc"
        TESTENV testenv/testUsdCatUnrecognizedExtension
        POST_COMMAND "${HEADBIN} -c8 output"
        POST_COMMAND_STDOUT_REDIRECT output_header.txt
        DIFF_COMPARE output_header.txt
        EXPECTED_RETURN_CODE 0
    )

    pxr_register_test(testUsdCatUnrecognizedExtension3
        COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdcat --flatten --skipSourceFileComment input.usda --out flattened --usdFormat usdc"
        TESTENV testenv/testUsdCatUnrecognizedExtension
        POST_COMMAND "${HEADBIN} -c8 flattened"
        POST_COMMAND_STDOUT_REDIRECT flattened_header.txt
        DIFF_COMPARE flattened_header.txt
        EXPECTED_RETURN_CODE 0
    )
endif()
