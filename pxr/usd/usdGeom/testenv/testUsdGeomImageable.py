#!/pxrpythonsubst
#
# Copyright 2019 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

from pxr import Usd, UsdGeom
import unittest

class TestUsdGeomImageable(unittest.TestCase):
    def test_MakeVisible(self):
        testFile = "AllInvisible.usda"
        s = Usd.Stage.Open(testFile)
        bar2 = UsdGeom.Imageable(s.GetPrimAtPath("/foo/bar2"))
        thing1 = UsdGeom.Imageable(s.GetPrimAtPath("/foo/bar1/thing1"))
        thing2 = UsdGeom.Imageable(s.GetPrimAtPath("/foo/bar1/thing2"))
        thing1.MakeVisible()
        self.assertEqual(bar2.ComputeVisibility(), UsdGeom.Tokens.invisible)
        self.assertEqual(thing1.ComputeVisibility(), UsdGeom.Tokens.inherited)
        self.assertEqual(thing2.ComputeVisibility(), UsdGeom.Tokens.invisible)

if __name__ == "__main__":
    unittest.main()
