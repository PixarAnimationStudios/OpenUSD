#!/pxrpythonsubst

import sys, os
from pxr import Gf, Usd, UsdGeom, Sdf, Tf

import Mentor.Runtime
from Mentor.Runtime import *

# Configure mentor so assertions terminate execution.
SetAssertMode(MTR_EXIT_TEST)

stage = Usd.Stage.CreateInMemory()
AssertTrue(stage)

model = UsdGeom.Xform.Define(stage, '/Model')
AssertTrue(model)
modelPrim = model.GetPrim()

modelPrim.SetMetadata('kind', 'prop')

modelSpace = Gf.Matrix4d(2.0).SetTranslate(Gf.Vec3d(10, 20, 30))
model.MakeMatrixXform().Set(modelSpace)

geomModel = UsdGeom.ModelAPI(modelPrim)
cnstrTarget = geomModel.CreateConstraintTarget("rootXf")
AssertTrue(cnstrTarget)

rootXf = cnstrTarget.GetAttr()
AssertTrue(rootXf)

cnstrTarget = geomModel.GetConstraintTarget("rootXf")
AssertTrue(cnstrTarget)

cnstrTarget.SetIdentifier('RootXf')
AssertEqual(cnstrTarget.GetIdentifier(), 'RootXf')

localConstraintSpace = Gf.Matrix4d(1.0).SetRotate(
    Gf.Rotation(Gf.Vec3d(1, 1, 0), 45))
cnstrTarget.Set(localConstraintSpace)

AssertEqual(cnstrTarget.ComputeInWorldSpace(Usd.TimeCode.Default()),
            localConstraintSpace * modelSpace)

# Test various types-- anything that is backed by Matrix4d is valid.
rootXf.SetTypeName(Sdf.ValueTypeNames.Double)
AssertFalse(cnstrTarget)
rootXf.SetTypeName(Sdf.ValueTypeNames.Frame4d)
AssertTrue(cnstrTarget)
rootXf.SetTypeName(Sdf.ValueTypeNames.Matrix4d)
AssertTrue(cnstrTarget)

invalidCnstrTarget = modelPrim.CreateAttribute(
    'invalidConstraintTargetName', Sdf.ValueTypeNames.Matrix4d)
AssertFalse(UsdGeom.ConstraintTarget(invalidCnstrTarget))

wrongTypeCnstrTarget = modelPrim.CreateAttribute(
    'constraintTargets:invalidTypeXf', Sdf.ValueTypeNames.Float)
AssertFalse(UsdGeom.ConstraintTarget(wrongTypeCnstrTarget))

# After removing model-ness, the constraint target is no longer valid
modelPrim.ClearMetadata('kind')
AssertFalse(cnstrTarget)
AssertFalse(cnstrTarget.IsValid())
AssertFalse(UsdGeom.ConstraintTarget(rootXf))

ExitTest()
