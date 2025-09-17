#!/pxrpythonsubst
#
# Copyright 2022 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

from pxr import Usd
from pxr import UsdGeom

import unittest


class TestUsdAbcXformPrimCollapsing(unittest.TestCase):
    def test_Read(self):
      testFile = 'testUsdAbcXformPrimCollapsing.abc'
      stage = Usd.Stage.Open(testFile)
      self.assertTrue(stage)

      basisCurves = UsdGeom.BasisCurves.Get(stage, "/CurvesAsset/Geom/Curve1")
      self.assertTrue(basisCurves)

      basisCurves = UsdGeom.BasisCurves.Get(stage, "/CurvesAsset/Geom/Curve2")
      self.assertTrue(basisCurves)

      basisCurves = UsdGeom.BasisCurves.Get(stage, "/CurvesAsset/Geom/Curve3")
      self.assertTrue(basisCurves)

      basisCurves = UsdGeom.BasisCurves.Get(stage, "/CurvesAsset/Geom/Curves456/CurveShape4")
      self.assertTrue(basisCurves)

      basisCurves = UsdGeom.BasisCurves.Get(stage, "/CurvesAsset/Geom/Curves456/CurveShape5")
      self.assertTrue(basisCurves)

      basisCurves = UsdGeom.BasisCurves.Get(stage, "/CurvesAsset/Geom/Curves456/CurveShape6")
      self.assertTrue(basisCurves)


if __name__ == '__main__':
    unittest.main()
