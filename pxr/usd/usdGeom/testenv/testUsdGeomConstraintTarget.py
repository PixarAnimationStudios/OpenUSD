#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

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
