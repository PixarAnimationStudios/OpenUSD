#!/pxrpythonsubst
#
# Copyright 2019 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

import sys, os, unittest
from pxr import Tf, Usd, UsdGeom

class TestUsdGeomMetrics(unittest.TestCase):

    def test_upAxis(self):
        stage = Usd.Stage.CreateInMemory()
        self.assertTrue(stage)

        self.assertEqual(UsdGeom.GetStageUpAxis(stage),
                         UsdGeom.GetFallbackUpAxis())
        
        self.assertTrue(UsdGeom.SetStageUpAxis(stage, UsdGeom.Tokens.y))
        self.assertEqual(UsdGeom.GetStageUpAxis(stage),
                         UsdGeom.Tokens.y)

        with Usd.EditContext(stage, Usd.EditTarget(stage.GetSessionLayer())):
            self.assertTrue(UsdGeom.SetStageUpAxis(stage, UsdGeom.Tokens.z))
        
        self.assertEqual(UsdGeom.GetStageUpAxis(stage),
                         UsdGeom.Tokens.z)
            
        # Setting illegal value should raise
        with self.assertRaises(Tf.ErrorException):
            UsdGeom.SetStageUpAxis(stage, UsdGeom.Tokens.x)


    def test_metersPerUnit(self):
        stage = Usd.Stage.CreateInMemory()
        self.assertTrue(stage)

        self.assertEqual(UsdGeom.GetStageMetersPerUnit(stage),
                         UsdGeom.LinearUnits.centimeters)
        self.assertFalse(UsdGeom.StageHasAuthoredMetersPerUnit(stage))

        self.assertTrue(UsdGeom.SetStageMetersPerUnit(stage,
                                                      UsdGeom.LinearUnits.feet))
        self.assertTrue(UsdGeom.StageHasAuthoredMetersPerUnit(stage))


        # test that common alternate representations that are not
        # numerically identical compare equal using LinearUnitsAre
        authored = UsdGeom.GetStageMetersPerUnit(stage)
        fromInches = 12 * UsdGeom.LinearUnits.inches
        self.assertNotEqual(authored, fromInches)
        self.assertTrue(UsdGeom.LinearUnitsAre(authored, fromInches))

        # similar test for feet to yards
        self.assertTrue(UsdGeom.SetStageMetersPerUnit(stage,
                                                      UsdGeom.LinearUnits.yards))
        authored = UsdGeom.GetStageMetersPerUnit(stage)
        fromFeet = 3 * UsdGeom.LinearUnits.feet
        self.assertNotEqual(authored, fromFeet)
        self.assertTrue(UsdGeom.LinearUnitsAre(authored, fromFeet))


if __name__ == "__main__":
    unittest.main()
