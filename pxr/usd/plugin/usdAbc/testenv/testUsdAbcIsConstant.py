#!/pxrpythonsubst
#
# Copyright 2020 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
from pxr import Usd, UsdGeom
import unittest

class TestUsdAbcIsConstant(unittest.TestCase):
    def test_Read(self):
      testFile = 'testUsdAbcIsConstant.abc'
      stage = Usd.Stage.Open(testFile)
      self.assertTrue(stage)

      animPoints = UsdGeom.Mesh.Get(stage, "/sphere_object1")
      self.assertTrue(animPoints)
      self.assertEqual(animPoints.GetPrim().GetProperty(UsdGeom.Tokens.points).GetNumTimeSamples(), 240)
      self.assertEqual(animPoints.GetOrderedXformOps()[0].GetNumTimeSamples(), 1)

      animXform = UsdGeom.Mesh.Get(stage, "/sphere_object2")
      self.assertTrue(animXform)
      self.assertEqual(animXform.GetPrim().GetProperty(UsdGeom.Tokens.points).GetNumTimeSamples(), 1)
      self.assertEqual(animXform.GetOrderedXformOps()[0].GetNumTimeSamples(), 240)

if __name__ == '__main__':
    unittest.main()
