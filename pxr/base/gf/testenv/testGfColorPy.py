#!/pxrpythonsubst
# 
# Copyright 2024 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license/.
#

import unittest
from pxr import Gf

def colorApproxEq(c1, c2):
    return Gf.IsClose(c1.GetRGB(), c2.GetRGB(), 1e-5)

class TestGfColor(unittest.TestCase):
    def setUp(self):
        self.csSRGB = Gf.ColorSpace(Gf.ColorSpaceNames.SRGB)
        self.csLinearSRGB = Gf.ColorSpace(Gf.ColorSpaceNames.LinearSRGB)
        self.csLinearRec709 = Gf.ColorSpace(Gf.ColorSpaceNames.LinearRec709)
        self.csG22Rec709 = Gf.ColorSpace(Gf.ColorSpaceNames.G22Rec709)
        self.csAp0 = Gf.ColorSpace(Gf.ColorSpaceNames.LinearAP0)
        self.csSRGBP3 = Gf.ColorSpace(Gf.ColorSpaceNames.SRGBDisplayP3)
        self.csLinearRec2020 = Gf.ColorSpace(Gf.ColorSpaceNames.LinearRec2020)
        self.csIdentity = Gf.ColorSpace(Gf.ColorSpaceNames.Identity)

        self.mauveLinear = Gf.Color(Gf.Vec3f(0.5, 0.25, 0.125), self.csLinearRec709)
        self.mauveGamma = Gf.Color(self.mauveLinear, self.csG22Rec709)

    def test_Repr(self):
        c = Gf.Color()
        self.assertEqual(c, eval(repr(c)))
        cs = Gf.ColorSpace("identity")
        self.assertEqual(cs, eval(repr(cs)))

    def test_DefaultConstruction(self):
        c = Gf.Color()
        self.assertEqual(c.GetColorSpace(), self.csLinearRec709)
        self.assertEqual(c.GetRGB(), Gf.Vec3f(0, 0, 0))

    def test_ConstructionWithColorSpace(self):
        c = Gf.Color(self.csSRGB)
        self.assertEqual(c.GetColorSpace(), self.csSRGB)
        self.assertEqual(c.GetRGB(), Gf.Vec3f(0, 0, 0))

    def test_ConstructionWithColorSpaceAndRgb(self):
        c = Gf.Color(Gf.Vec3f(0.5, 0.5, 0.5), self.csSRGB)
        self.assertEqual(c.GetColorSpace(), self.csSRGB)
        self.assertEqual(c.GetRGB(), Gf.Vec3f(0.5, 0.5, 0.5))

    def test_EotfCurveLinear(self):
        c1 = Gf.Color(self.mauveLinear, self.csSRGB)  # convert linear to SRGB
        c2 = Gf.Color(c1, self.csLinearSRGB)
        self.assertTrue(Gf.IsClose(self.mauveLinear, c2, 1e-5))
        c3 = Gf.Color(c2, self.csSRGB)
        self.assertTrue(Gf.IsClose(c1, c3, 1e-5))

    def test_RoundTrippingToRec2020(self):
        c1 = Gf.Color(self.mauveLinear, self.csLinearRec2020)
        c2 = Gf.Color(c1, self.csLinearSRGB)
        self.assertTrue(Gf.IsClose(self.mauveLinear, c2, 1e-5))

    def test_ConstructionWithConversion(self):
        colG22Rec709 = Gf.Color(self.mauveLinear, self.csG22Rec709)
        self.assertTrue(Gf.IsClose(colG22Rec709, self.mauveGamma, 1e-5))
        colLinRec709 = Gf.Color(colG22Rec709, self.csLinearRec709)
        self.assertTrue(Gf.IsClose(colLinRec709, self.mauveLinear, 1e-5))

        self.assertEqual(colG22Rec709.GetColorSpace(), self.csG22Rec709)
        self.assertEqual(colLinRec709.GetColorSpace(), self.csLinearRec709)

        colSRGB_2 = Gf.Color(colLinRec709, self.csSRGB)
        colAp0 = Gf.Color(colSRGB_2, self.csAp0)
        colSRGB_3 = Gf.Color(colAp0, self.csSRGB)
        col_SRGBP3 = Gf.Color(colSRGB_3, self.csSRGBP3)
        colLinRec709_2 = Gf.Color(col_SRGBP3, self.csLinearRec709)
        self.assertTrue(Gf.IsClose(colLinRec709_2, colLinRec709, 1e-5))

    def test_MoveConstructor(self):
        c1 = Gf.Color(Gf.Vec3f(0.5, 0.25, 0.125), self.csAp0)
        c2 = Gf.Color(c1)  # Python doesn't have move semantics, but this tests copying
        self.assertEqual(c2.GetColorSpace(), self.csAp0)
        self.assertTrue(Gf.IsClose(c2.GetRGB(), Gf.Vec3f(0.5, 0.25, 0.125), 1e-5))

    def test_CopyAssignment(self):
        c1 = Gf.Color(Gf.Vec3f(0.5, 0.25, 0.125), self.csAp0)
        c2 = Gf.Color()
        c2 = c1
        self.assertEqual(c2.GetColorSpace(), self.csAp0)
        self.assertTrue(Gf.IsClose(c2.GetRGB(), Gf.Vec3f(0.5, 0.25, 0.125), 1e-5))

    def test_Comparison(self):
        c1 = Gf.Color(Gf.Vec3f(0.5, 0.25, 0.125), self.csAp0)
        c2 = Gf.Color(Gf.Vec3f(0.5, 0.25, 0.125), self.csAp0)
        self.assertTrue(Gf.IsClose(c1, c2, 1e-5))
        self.assertEqual(c1.GetColorSpace(), c2.GetColorSpace())
        self.assertTrue(colorApproxEq(c1, c2))

if __name__ == '__main__':
    unittest.main()
