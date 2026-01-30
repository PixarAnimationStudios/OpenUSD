#
# Copyright 2025 Apple
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usd)

pxr_cpp_bin(usdcrush
        LIBRARIES
        tf
        sdf
        usd
        usdUtils
        usdPmc
        )

pxr_test_scripts(
    testenv/testUsdCrush.py
)

pxr_register_test(testUsdCrush
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdCrush"
    EXPECTED_RETURN_CODE 0
    PRE_PATH
        ${CMAKE_INSTALL_PREFIX}/bin
)

