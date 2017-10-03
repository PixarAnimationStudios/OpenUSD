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

import sys, os, unittest
from pxr import Usd, UsdGeom, Sdf, Tf

class TestUsdGeomConstraintTarget(unittest.TestCase):
    def test_Basic(self):
        stage = Usd.Stage.CreateInMemory()
        self.assertTrue(stage)

        # When velocityScale is authored on parent root, child that does
        # not author (Child1) inherits from parent, but child that does
        # author computes its own value (Child2)
        root1 = UsdGeom.Xform.Define(stage, '/root1')
        UsdGeom.MotionAPI(root1).CreateVelocityScaleAttr(0.5)
        root1Child1 = UsdGeom.Mesh.Define(stage, '/root1/mesh1')
        self.assertEqual(UsdGeom.MotionAPI(root1Child1).ComputeVelocityScale(),
                         0.5)

        root1Child2 = UsdGeom.Mesh.Define(stage, '/root1/mesh2')
        UsdGeom.MotionAPI(root1Child2).CreateVelocityScaleAttr(2.0)
        self.assertEqual(UsdGeom.MotionAPI(root1Child2).ComputeVelocityScale(),
                         2.0)

        # When nothing is authord anywhere, fallback is 1.0
        root2 = UsdGeom.Xform.Define(stage, '/root2')
        root2Child = UsdGeom.Mesh.Define(stage, '/root2/mesh')
        self.assertEqual(UsdGeom.MotionAPI(root2Child).ComputeVelocityScale(),
                         1.0)


if __name__ == "__main__":
    unittest.main()
