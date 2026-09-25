#
# Copyright 2026 Apple
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usd)

pxr_cpp_bin(usdcrush
        LIBRARIES
        tf
        usdPmc
        )
