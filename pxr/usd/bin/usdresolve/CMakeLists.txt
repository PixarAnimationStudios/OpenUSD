set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usd)

pxr_python_bin(usdresolve
    DEPENDENCIES
        ar
        sdf
        usd
)

pxr_install_test_dir(
    SRC testenv/testResolveRelPath
    DEST testResolveRelPath
)

pxr_register_test(testResolveRelPath
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdresolve input.usda"
    STDOUT_REDIRECT output.txt
    DIFF_COMPARE output.txt
    CLEAN_OUTPUT "(?:[A-Za-z]:)?\\/.*\\/"
    EXPECTED_RETURN_CODE 0
)

