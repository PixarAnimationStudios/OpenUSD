#!/pxrpythonsubst
#
# Copyright 2023 Pixar
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

import sys, unittest
from pxr import Sdf, Usd, UsdGeom, UsdShade

class TestUsdGeomDashDotLines(unittest.TestCase):
    def test_InterpolationTypes(self):
        stage = Usd.Stage.Open('dashDotLines.usda')
        t = Usd.TimeCode.Default()
        c = UsdGeom.DashDotLines.Get(stage, '/StyledPolyline')
        # Test attributes.
        self.assertTrue(c.GetBoolAttr(c.GetScreenSpacePatternAttr(), t))
        self.assertEqual(c.GetFloatAttr(c.GetPatternScaleAttr(), t), 5.0)
        self.assertEqual(c.GetTokenAttr(c.GetStartCapTypeAttr(), t), UsdGeom.Tokens.round)
        self.assertEqual(c.GetTokenAttr(c.GetEndCapTypeAttr(), t), UsdGeom.Tokens.triangle)

        # Test dashDotPattern inheritance.
        prim = c.GetPrim()
        patternPeriodAttr = prim.GetAttribute(UsdGeom.Tokens.patternPeriod)
        self.assertEqual(patternPeriodAttr.Get(), 20.0)
        patternAttr = prim.GetAttribute(UsdGeom.Tokens.pattern)
        float2Array = patternAttr.Get()
        self.assertEqual(float2Array[0][0], 0)
        self.assertEqual(float2Array[0][1], 10)
        self.assertEqual(float2Array[1][0], 5)
        self.assertEqual(float2Array[1][1], 0)

if __name__ == '__main__':
    unittest.main()
