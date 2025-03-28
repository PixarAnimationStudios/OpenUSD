#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

from pxr import Sdf, Usd, UsdAbc
import unittest

class TestUsdAbcBugs(unittest.TestCase):
    # ========================================================================
    # Bug 107381
    #
    # Write primvars to top level if prim doesn't have .arbGeomParams property.
    # ========================================================================
    def test_Bug107381(self):
        layer = Sdf.Layer.FindOrOpen('bug107381.usd')
        self.assertTrue(layer)
        layer.ExportToString()

if __name__ == "__main__":
    unittest.main()
