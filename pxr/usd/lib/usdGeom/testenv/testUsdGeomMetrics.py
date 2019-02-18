#!/pxrpythonsubst
#
# Copyright 2019 Pixar
#
# Licensed under the Apache License, Version 2.0 (the "Apache License")
# with the following modification; you may not use this file except in
# compliance with the Apache License and the following modification to it:
# Section 6. Trademarks. is deleted and replaced with:
#
# 6. Trademarks. This License does not grant permission to use the trade
#    names, trademarks, service marks, or product names of the Licensor
#    and its affiliates, except as required to comply with Section 4(c) of
#    the License and to reproduce the content of the NOTICE file.
#
# You may obtain a copy of the Apache License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the Apache License with the above modification is
# distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied. See the Apache License for the specific
# language governing permissions and limitations under the Apache License.

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


if __name__ == "__main__":
    unittest.main()
