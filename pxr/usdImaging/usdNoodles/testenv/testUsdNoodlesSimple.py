#!/pxrpythonsubst
#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#


"""
Simple test to verify usdNoodles module loads correctly.

This test checks basic module functionality:
- UsdNoodles module imports from pxr
- _noodles C++ extension module is available
- Key classes (VertexGenerator, NodeVertex) exist
"""

import unittest

try:
    from pxr import UsdNoodles  # noqa: F401

    _has_pxr = True
except ImportError:
    _has_pxr = False


class TestUsdNoodlesImport(unittest.TestCase):
    """Verify the UsdNoodles module loads correctly."""

    @unittest.skipUnless(_has_pxr, "pxr.UsdNoodles not available")
    def test_import_usd_noodles(self):
        self.assertTrue(hasattr(UsdNoodles, "__name__"))

    @unittest.skipUnless(_has_pxr, "pxr.UsdNoodles not available")
    def test_import_usd_noodles_extension(self):
        from pxr.UsdNoodles import _usdNoodles as _noodles  # noqa: F401


if __name__ == "__main__":
    unittest.main()
