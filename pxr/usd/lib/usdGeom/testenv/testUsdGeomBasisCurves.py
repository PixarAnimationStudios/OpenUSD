#!/pxrpythonsubst
#
# Copyright 2017 Pixar
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
from pxr import Usd, UsdGeom

class TestUsdGeomBasisCurves(unittest.TestCase):
    def test_InterpolationTypes(self):
        stage = Usd.Stage.Open('basisCurves.usda')
        t = Usd.TimeCode.Default()
        c = UsdGeom.BasisCurves.Get(stage, '/BezierCubic')
        self.assertEqual(c.ComputeUniformDataSize(t), 2)
        self.assertEqual(c.ComputeVaryingDataSize(t), 4)
        self.assertEqual(c.ComputeVertexDataSize(t), 10)
        self.assertEqual(c.ComputeInterpolationForSize(1, t), UsdGeom.Tokens.constant)
        self.assertEqual(c.ComputeInterpolationForSize(2, t), UsdGeom.Tokens.uniform)
        self.assertEqual(c.ComputeInterpolationForSize(4, t), UsdGeom.Tokens.varying)
        self.assertEqual(c.ComputeInterpolationForSize(10, t), UsdGeom.Tokens.vertex)
        self.assertFalse(c.ComputeInterpolationForSize(100, t))
        self.assertFalse(c.ComputeInterpolationForSize(0, t))

if __name__ == '__main__':
    import os
    print os.listdir(os.getcwd())
    print os.getcwd()
    unittest.main()
