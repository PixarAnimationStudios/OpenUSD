#!/pxrpythonsubst
#
# Copyright 2021 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

import sys, os, unittest
from pxr import Tf, Usd, UsdValidation, UsdPhysics, UsdGeom, Gf


class TestUsdPhysicsValidation(unittest.TestCase):

    def test_rigid_body_xformable(self):        
        validationRegistry = UsdValidation.ValidationRegistry()
        validator = validationRegistry.GetOrLoadValidatorByName(
            "usdPhysicsValidators:RigidBodyChecker"
        )

        self.assertTrue(validator)

        stage = Usd.Stage.CreateInMemory()
        self.assertTrue(stage)

        rigidbody = UsdGeom.Scope.Define(stage, "/rigidBody")
        UsdPhysics.RigidBodyAPI.Apply(rigidbody.GetPrim())

        errors = validator.Validate(rigidbody.GetPrim())
        self.assertTrue(len(errors) == 1)        
        self.assertTrue(errors[0].GetName() == "RigidBodyNonXformable")

    def test_rigid_body_orientation_scale(self):        
        validationRegistry = UsdValidation.ValidationRegistry()
        validator = validationRegistry.GetOrLoadValidatorByName(
            "usdPhysicsValidators:RigidBodyChecker"
        )

        self.assertTrue(validator)

        stage = Usd.Stage.CreateInMemory()
        self.assertTrue(stage)

        rigidbody = UsdGeom.Xform.Define(stage, "/rigidBody")
        UsdPhysics.RigidBodyAPI.Apply(rigidbody.GetPrim())

        errors = validator.Validate(rigidbody.GetPrim())
        self.assertTrue(len(errors) == 0)

        transform = Gf.Transform()
        transform.SetScale(Gf.Vec3d(7,8,9))
        transform.SetPivotOrientation(Gf.Rotation(Gf.Vec3d(1,2,3), 20.3))

        matrix = transform.GetMatrix()
        rigidbody.AddTransformOp().Set(matrix)

        errors = validator.Validate(rigidbody.GetPrim())
        self.assertTrue(len(errors) == 1)        
        self.assertTrue(errors[0].GetName() == "RigidBodyOrientationScale")        

    def test_rigid_body_nesting(self):        
        validationRegistry = UsdValidation.ValidationRegistry()
        validator = validationRegistry.GetOrLoadValidatorByName(
            "usdPhysicsValidators:RigidBodyChecker"
        )

        self.assertTrue(validator)

        stage = Usd.Stage.CreateInMemory()
        self.assertTrue(stage)

        rigidbody0 = UsdGeom.Xform.Define(stage, "/rigidBody0")
        UsdPhysics.RigidBodyAPI.Apply(rigidbody0.GetPrim())

        rigidbody1 = UsdGeom.Xform.Define(stage, "/rigidBody0/rigidBody1")
        UsdPhysics.RigidBodyAPI.Apply(rigidbody1.GetPrim())

        errors = validator.Validate(rigidbody0.GetPrim())
        self.assertTrue(len(errors) == 0)

        errors = validator.Validate(rigidbody1.GetPrim())
        self.assertTrue(len(errors) == 1)        
        self.assertTrue(errors[0].GetName() == "NestedRigidBody")

    def test_rigid_body_instancing(self):        
        validationRegistry = UsdValidation.ValidationRegistry()
        validator = validationRegistry.GetOrLoadValidatorByName(
            "usdPhysicsValidators:RigidBodyChecker"
        )

        self.assertTrue(validator)

        stage = Usd.Stage.CreateInMemory()
        self.assertTrue(stage)

        UsdGeom.Xform.Define(stage, "/xform")
        rigidbody = UsdGeom.Cube.Define(stage, "/xform/rigidBody")
        UsdPhysics.RigidBodyAPI.Apply(rigidbody.GetPrim())

        xform = UsdGeom.Xform.Define(stage, "/xformInstance")
        xform.GetPrim().GetReferences().AddInternalReference("/xform")
        xform.GetPrim().SetInstanceable(True)

        errors = validator.Validate(rigidbody.GetPrim())
        self.assertTrue(len(errors) == 0)
        
        instanceRigidBody = stage.GetPrimAtPath("/xformInstance/rigidBody")
        self.assertTrue(instanceRigidBody.IsInstanceProxy())        

        errors = validator.Validate(instanceRigidBody.GetPrim())
        self.assertTrue(len(errors) == 1)        
        self.assertTrue(errors[0].GetName() == "RigidBodyNonInstanceable")

    def test_articulation_nesting(self):        
        validationRegistry = UsdValidation.ValidationRegistry()
        validator = validationRegistry.GetOrLoadValidatorByName(
            "usdPhysicsValidators:ArticulationChecker"
        )

        self.assertTrue(validator)

        stage = Usd.Stage.CreateInMemory()
        self.assertTrue(stage)

        articulation0 = UsdGeom.Xform.Define(stage, "/articulation0")
        UsdPhysics.ArticulationRootAPI.Apply(articulation0.GetPrim())

        articulation1 = UsdGeom.Xform.Define(stage, "/articulation0/articulation1")
        UsdPhysics.ArticulationRootAPI.Apply(articulation1.GetPrim())

        errors = validator.Validate(articulation0.GetPrim())
        self.assertTrue(len(errors) == 0)

        errors = validator.Validate(articulation1.GetPrim())
        self.assertTrue(len(errors) == 1)        
        self.assertTrue(errors[0].GetName() == "NestedArticulation")        

    def test_articulation_body(self):        
        validationRegistry = UsdValidation.ValidationRegistry()
        validator = validationRegistry.GetOrLoadValidatorByName(
            "usdPhysicsValidators:ArticulationChecker"
        )

        self.assertTrue(validator)

        stage = Usd.Stage.CreateInMemory()
        self.assertTrue(stage)

        articulation = UsdGeom.Xform.Define(stage, "/articulation")
        UsdPhysics.ArticulationRootAPI.Apply(articulation.GetPrim())

        rboAPI = UsdPhysics.RigidBodyAPI.Apply(articulation.GetPrim())

        errors = validator.Validate(articulation.GetPrim())
        self.assertTrue(len(errors) == 0)

        rboAPI.GetRigidBodyEnabledAttr().Set(False)

        errors = validator.Validate(articulation.GetPrim())
        self.assertTrue(len(errors) == 1)        
        self.assertTrue(errors[0].GetName() == "ArticulationOnStaticBody")        

        rboAPI.GetRigidBodyEnabledAttr().Set(True)
        rboAPI.GetKinematicEnabledAttr().Set(True)

        errors = validator.Validate(articulation.GetPrim())
        self.assertTrue(len(errors) == 1)        
        self.assertTrue(errors[0].GetName() == "ArticulationOnKinematicBody")        

    def test_physics_joint_invalid_rel(self):
        validationRegistry = UsdValidation.ValidationRegistry()
        validator = validationRegistry.GetOrLoadValidatorByName(
            "usdPhysicsValidators:PhysicsJointChecker"
        )

        self.assertTrue(validator)

        stage = Usd.Stage.CreateInMemory()
        self.assertTrue(stage)

        physicsJoint = UsdPhysics.Joint.Define(stage, "/joint")

        physicsJoint.GetBody1Rel().AddTarget("/invalidPrim")

        errors = validator.Validate(physicsJoint.GetPrim())
        self.assertTrue(len(errors) == 1)
        self.assertTrue(errors[0].GetName() == "JointInvalidPrimRel")        

    def test_physics_joint_multiple_rels(self):
        validationRegistry = UsdValidation.ValidationRegistry()
        validator = validationRegistry.GetOrLoadValidatorByName(
            "usdPhysicsValidators:PhysicsJointChecker"
        )

        self.assertTrue(validator)

        stage = Usd.Stage.CreateInMemory()
        self.assertTrue(stage)

        UsdGeom.Xform.Define(stage, "/xform0")
        UsdGeom.Xform.Define(stage, "/xform1")

        physicsJoint = UsdPhysics.Joint.Define(stage, "/joint")

        physicsJoint.GetBody1Rel().AddTarget("/xform0")

        errors = validator.Validate(physicsJoint.GetPrim())
        self.assertTrue(len(errors) == 0)

        physicsJoint.GetBody1Rel().AddTarget("/xform1")

        errors = validator.Validate(physicsJoint.GetPrim())
        self.assertTrue(len(errors) == 1)
        self.assertTrue(errors[0].GetName() == "JointMultiplePrimsRel")

    def test_collider_non_uniform_scale(self):
        validationRegistry = UsdValidation.ValidationRegistry()
        validator = validationRegistry.GetOrLoadValidatorByName(
            "usdPhysicsValidators:ColliderChecker"
        )

        self.assertTrue(validator)

        stage = Usd.Stage.CreateInMemory()
        self.assertTrue(stage)

        shapes = [ UsdGeom.Sphere, UsdGeom.Capsule, UsdGeom.Cone, UsdGeom.Cylinder ]

        for shapeType in shapes:
            shape = shapeType.Define(stage, "/shape")
            UsdPhysics.CollisionAPI.Apply(shape.GetPrim())

            errors = validator.Validate(shape.GetPrim())
            self.assertTrue(len(errors) == 0)

            shape.AddScaleOp().Set(Gf.Vec3d(1,2,3))

            errors = validator.Validate(shape.GetPrim())
            self.assertTrue(len(errors) == 1)
            self.assertTrue(errors[0].GetName() == "ColliderNonUniformScale")

            stage.RemovePrim(shape.GetPrim().GetPrimPath())


    def test_points_collider(self):
        validationRegistry = UsdValidation.ValidationRegistry()
        validator = validationRegistry.GetOrLoadValidatorByName(
            "usdPhysicsValidators:ColliderChecker"
        )

        self.assertTrue(validator)

        stage = Usd.Stage.CreateInMemory()
        self.assertTrue(stage)

        shape = UsdGeom.Points.Define(stage, "/shape")
        UsdPhysics.CollisionAPI.Apply(shape.GetPrim())

        shape.GetWidthsAttr().Set([1])
        shape.GetPointsAttr().Set([Gf.Vec3f(1.0)])

        errors = validator.Validate(shape.GetPrim())
        self.assertTrue(len(errors) == 0)

        shape.GetWidthsAttr().Set([])
        shape.GetPointsAttr().Set([Gf.Vec3f(1.0)])

        errors = validator.Validate(shape.GetPrim())
        self.assertTrue(len(errors) == 1)
        self.assertTrue(errors[0].GetName() == "ColliderSpherePointsDataMissing")

        shape.GetWidthsAttr().Set([1])
        shape.GetPointsAttr().Set([])

        errors = validator.Validate(shape.GetPrim())
        self.assertTrue(len(errors) == 1)
        self.assertTrue(errors[0].GetName() == "ColliderSpherePointsDataMissing")

        shape.GetWidthsAttr().Set([1,3])
        shape.GetPointsAttr().Set([Gf.Vec3f(1.0)])

        errors = validator.Validate(shape.GetPrim())
        self.assertTrue(len(errors) == 1)
        self.assertTrue(errors[0].GetName() == "ColliderSpherePointsDataMissing")

        shape.AddScaleOp().Set(Gf.Vec3d(1,2,3))
        shape.GetWidthsAttr().Set([1])
        shape.GetPointsAttr().Set([Gf.Vec3f(1.0)])

        errors = validator.Validate(shape.GetPrim())
        self.assertTrue(len(errors) == 1)
        self.assertTrue(errors[0].GetName() == "ColliderNonUniformScale")

        stage.RemovePrim(shape.GetPrim().GetPrimPath())

if __name__ == "__main__":
    unittest.main()
