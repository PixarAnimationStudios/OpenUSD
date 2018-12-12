set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usd)

pxr_python_bin(usdresolve
    DEPENDENCIES
        ar
        sdf
        usd
)

# XXX:
# Temporarily disable this test on Windows; the output contains
# a file path but the specified regex isn't stripping off the
# directory as expected.
if (NOT WIN32)
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
endif()
