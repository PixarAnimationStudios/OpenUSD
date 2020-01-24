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
from pxr import Gf, Usd, UsdGeom, Sdf, Tf

class TestUsdGeomConstraintTarget(unittest.TestCase):
    def test_Basic(self):
        stage = Usd.Stage.CreateInMemory()
        self.assertTrue(stage)

        model = UsdGeom.Xform.Define(stage, '/Model')
        self.assertTrue(model)
        modelPrim = model.GetPrim()

        modelPrim.SetMetadata('kind', 'component')

        modelSpace = Gf.Matrix4d(2.0).SetTranslate(Gf.Vec3d(10, 20, 30))
        model.MakeMatrixXform().Set(modelSpace)

        geomModel = UsdGeom.ModelAPI(modelPrim)
        cnstrTarget = geomModel.CreateConstraintTarget("rootXf")
        self.assertTrue(cnstrTarget)

        rootXf = cnstrTarget.GetAttr()
        self.assertTrue(rootXf)

        cnstrTarget = geomModel.GetConstraintTarget("rootXf")
        self.assertTrue(cnstrTarget)

        cnstrTarget.SetIdentifier('RootXf')
        self.assertEqual(cnstrTarget.GetIdentifier(), 'RootXf')

        localConstraintSpace = Gf.Matrix4d(1.0).SetRotate(
            Gf.Rotation(Gf.Vec3d(1, 1, 0), 45))
        cnstrTarget.Set(localConstraintSpace)

        self.assertEqual(cnstrTarget.ComputeInWorldSpace(Usd.TimeCode.Default()),
                    localConstraintSpace * modelSpace)

        # Test various types-- anything that is backed by Matrix4d is valid.
        rootXf.SetTypeName(Sdf.ValueTypeNames.Double)
        self.assertFalse(cnstrTarget)
        rootXf.SetTypeName(Sdf.ValueTypeNames.Frame4d)
        self.assertTrue(cnstrTarget)
        rootXf.SetTypeName(Sdf.ValueTypeNames.Matrix4d)
        self.assertTrue(cnstrTarget)

        invalidCnstrTarget = modelPrim.CreateAttribute(
            'invalidConstraintTargetName', Sdf.ValueTypeNames.Matrix4d)
        self.assertFalse(UsdGeom.ConstraintTarget(invalidCnstrTarget))

        wrongTypeCnstrTarget = modelPrim.CreateAttribute(
            'constraintTargets:invalidTypeXf', Sdf.ValueTypeNames.Float)
        self.assertFalse(UsdGeom.ConstraintTarget(wrongTypeCnstrTarget))

        # After removing model-ness, the constraint target is no longer valid
        modelPrim.ClearMetadata('kind')
        self.assertFalse(cnstrTarget)
        self.assertFalse(cnstrTarget.IsValid())
        self.assertFalse(UsdGeom.ConstraintTarget(rootXf))

if __name__ == "__main__":
    unittest.main()
