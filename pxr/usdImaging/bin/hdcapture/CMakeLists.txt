set(PXR_PREFIX pxr/usdImaging)
set(PXR_PACKAGE HdUsdWriter)

pxr_python_bin(hdcapture
    DEPENDENCIES
        usd
        usdAppUtils)

pxr_install_test_dir(
    SRC testenv/testUsdWriterBaseline
    DEST testUsdWriterBaseline
)

pxr_install_test_dir(
    SRC testenv/testUsdWriterProtoInstancing
    DEST testUsdWriterProtoInstancing
)

pxr_install_test_dir(
    SRC testenv/testUsdWriterMaterialAssignToInstanced
    DEST testUsdWriterMaterialAssignToInstanced
)

pxr_test_scripts(
    testenv/testUsdWriterBaseline.py
)

pxr_test_scripts(
    testenv/testUsdWriterProtoInstancing.py
)

pxr_test_scripts(
    testenv/testUsdWriterMaterialAssignToInstanced.py
)

pxr_register_test(testUsdWriterBaseline
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdWriterBaseline"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdWriterProtoInstancing
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdWriterProtoInstancing"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdWriterMaterialAssignToInstanced
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdWriterMaterialAssignToInstanced"
    EXPECTED_RETURN_CODE 0
)
