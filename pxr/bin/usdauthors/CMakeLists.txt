set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usd)

pxr_cpp_bin(usdauthors
    LIBRARIES
        tf
        sdf
        usd
        usdMedia
        ar
)

pxr_register_test(testUsdAuthors
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdauthors input.usda"
    TESTENV testenv/testUsdAuthors
    STDOUT_REDIRECT output.txt
    DIFF_COMPARE output.txt
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdAuthorsDeep
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdauthors --deep input.usda"
    TESTENV testenv/testUsdAuthors
    STDOUT_REDIRECT output_deep.txt
    DIFF_COMPARE output_deep.txt
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdAuthorsSummary
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdauthors --summary input.usda"
    TESTENV testenv/testUsdAuthors
    STDOUT_REDIRECT output_summary.txt
    DIFF_COMPARE output_summary.txt
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdAuthorsClobbered
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdauthors --deep clobber.usda"
    TESTENV testenv/testUsdAuthors
    STDOUT_REDIRECT output_clobber.txt
    DIFF_COMPARE output_clobber.txt
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdAuthorsUnloaded
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdauthors --unloaded payloaded.usda"
    TESTENV testenv/testUsdAuthors
    STDOUT_REDIRECT output_unloaded.txt
    DIFF_COMPARE output_unloaded.txt
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdAuthorsLayer
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdauthors --layer input.usda"
    TESTENV testenv/testUsdAuthors
    STDOUT_REDIRECT output_layer.txt
    DIFF_COMPARE output_layer.txt
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdAuthorsLayerVariants
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdauthors --layer variants.usda"
    TESTENV testenv/testUsdAuthors
    STDOUT_REDIRECT output_layer_variants.txt
    DIFF_COMPARE output_layer_variants.txt
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdAuthorsVariantsComposed
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdauthors --deep variants.usda"
    TESTENV testenv/testUsdAuthors
    STDOUT_REDIRECT output_variants_deep.txt
    DIFF_COMPARE output_variants_deep.txt
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdAuthorsNoRecords
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdauthors empty.usda"
    TESTENV testenv/testUsdAuthors
    STDOUT_REDIRECT output_empty.txt
    DIFF_COMPARE output_empty.txt
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdAuthorsMissingInput
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdauthors nonexistent.usda"
    EXPECTED_RETURN_CODE 1
)
