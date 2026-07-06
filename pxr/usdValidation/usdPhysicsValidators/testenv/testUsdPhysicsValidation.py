#!/pxrpythonsubst
#
# Copyright 2021 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

import sys, os, unittest
from pxr import Tf, Usd, UsdValidation, UsdPhysics, UsdGeom, Gf, Sdf, Vt


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
        self.assertTrue(len(errors) == 0)

    def test_physics_joint_invalid_rel(self):
        validationRegistry = UsdValidation.ValidationRegistry()
        validator = validationRegistry.GetOrLoadValidatorByName(
            "usdPhysicsValidators:PhysicsJointChecker"
        )

        self.assertTrue(validator)

        stage = Usd.Stage.CreateInMemory()
        self.assertTrue(stage)

        body0 = UsdGeom.Xform.Define(stage, "/body0")
        UsdPhysics.RigidBodyAPI.Apply(body0.GetPrim())

        physicsJoint = UsdPhysics.Joint.Define(stage, "/joint")
        physicsJoint.GetBody0Rel().AddTarget("/body0")
        physicsJoint.GetBody1Rel().AddTarget("/invalidPrim")

        errors = validator.Validate(physicsJoint.GetPrim())
        self.assertTrue(len(errors) == 1)
        self.assertTrue(errors[0].GetName() == "JointInvalidPrimRel")

    def test_physics_joint_rel_not_xformable(self):
        validationRegistry = UsdValidation.ValidationRegistry()
        validator = validationRegistry.GetOrLoadValidatorByName(
            "usdPhysicsValidators:PhysicsJointChecker"
        )

        self.assertTrue(validator)

        stage = Usd.Stage.CreateInMemory()
        self.assertTrue(stage)

        UsdGeom.Scope.Define(stage, "/scope")
        xform = UsdGeom.Xform.Define(stage, "/xform")
        UsdPhysics.RigidBodyAPI.Apply(xform.GetPrim())

        physicsJoint = UsdPhysics.Joint.Define(stage, "/joint")
        physicsJoint.GetBody0Rel().AddTarget("/scope")
        physicsJoint.GetBody1Rel().AddTarget("/xform")

        errors = validator.Validate(physicsJoint.GetPrim())
        self.assertTrue(len(errors) == 1)
        self.assertTrue(errors[0].GetName() == "JointRelNotXformable")

        # Xform is Xformable — should not trigger JointRelNotXformable
        physicsJoint.GetBody0Rel().SetTargets(["/xform"])

        errors = validator.Validate(physicsJoint.GetPrim())
        self.assertTrue(len(errors) == 0)

    def test_physics_joint_requires_enabled_rigid_body(self):
        validationRegistry = UsdValidation.ValidationRegistry()
        validator = validationRegistry.GetOrLoadValidatorByName(
            "usdPhysicsValidators:PhysicsJointChecker"
        )

        self.assertTrue(validator)

        stage = Usd.Stage.CreateInMemory()
        self.assertTrue(stage)

        # Joint with no body rels — no enabled rigid body
        physicsJoint = UsdPhysics.Joint.Define(stage, "/joint")

        errors = validator.Validate(physicsJoint.GetPrim())
        self.assertTrue(len(errors) == 1)
        self.assertTrue(errors[0].GetName() == "JointNoEnabledRigidBody")

        # Add one enabled rigid body — should clear the error
        body0 = UsdGeom.Xform.Define(stage, "/body0")
        rbo0 = UsdPhysics.RigidBodyAPI.Apply(body0.GetPrim())
        physicsJoint.GetBody0Rel().AddTarget("/body0")

        errors = validator.Validate(physicsJoint.GetPrim())
        self.assertTrue(len(errors) == 0)

        # Disable the rigid body — should fail again
        body1 = UsdGeom.Xform.Define(stage, "/body1")
        rbo1 = UsdPhysics.RigidBodyAPI.Apply(body1.GetPrim())
        rbo0.GetRigidBodyEnabledAttr().Set(False)
        rbo1.GetRigidBodyEnabledAttr().Set(False)
        physicsJoint.GetBody1Rel().AddTarget("/body1")

        errors = validator.Validate(physicsJoint.GetPrim())
        self.assertTrue(len(errors) == 1)
        self.assertTrue(errors[0].GetName() == "JointNoEnabledRigidBody")

    def test_physics_joint_multiple_rels(self):
        validationRegistry = UsdValidation.ValidationRegistry()
        validator = validationRegistry.GetOrLoadValidatorByName(
            "usdPhysicsValidators:PhysicsJointChecker"
        )

        self.assertTrue(validator)

        stage = Usd.Stage.CreateInMemory()
        self.assertTrue(stage)

        xform0 = UsdGeom.Xform.Define(stage, "/xform0")
        UsdPhysics.RigidBodyAPI.Apply(xform0.GetPrim())
        xform1 = UsdGeom.Xform.Define(stage, "/xform1")
        UsdPhysics.RigidBodyAPI.Apply(xform1.GetPrim())

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

    def test_points_collider_primvar_widths(self):
        validationRegistry = UsdValidation.ValidationRegistry()
        validator = validationRegistry.GetOrLoadValidatorByName(
            "usdPhysicsValidators:ColliderChecker"
        )

        self.assertTrue(validator)

        # only widths attr authored, matching count — should pass
        stage = Usd.Stage.CreateInMemory()
        shape = UsdGeom.Points.Define(stage, "/shape")
        UsdPhysics.CollisionAPI.Apply(shape.GetPrim())
        shape.GetPointsAttr().Set([Gf.Vec3f(1.0), Gf.Vec3f(2.0)])
        shape.GetWidthsAttr().Set([1.0, 2.0])

        errors = validator.Validate(shape.GetPrim())
        self.assertTrue(len(errors) == 0)

        # only primvars:widths authored, matching count — should pass
        stage = Usd.Stage.CreateInMemory()
        shape = UsdGeom.Points.Define(stage, "/shape")
        UsdPhysics.CollisionAPI.Apply(shape.GetPrim())
        shape.GetPointsAttr().Set([Gf.Vec3f(1.0), Gf.Vec3f(2.0)])
        primvarsAPI = UsdGeom.PrimvarsAPI(shape.GetPrim())
        widthsPv = primvarsAPI.CreatePrimvar(
            "widths", Sdf.ValueTypeNames.FloatArray)
        widthsPv.Set([1.0, 2.0])

        errors = validator.Validate(shape.GetPrim())
        self.assertTrue(len(errors) == 0)

        # both authored, primvars:widths wins — widths attr has wrong count
        # but primvars:widths matches, so should pass
        stage = Usd.Stage.CreateInMemory()
        shape = UsdGeom.Points.Define(stage, "/shape")
        UsdPhysics.CollisionAPI.Apply(shape.GetPrim())
        shape.GetPointsAttr().Set([Gf.Vec3f(1.0), Gf.Vec3f(2.0)])
        shape.GetWidthsAttr().Set([1.0])
        primvarsAPI = UsdGeom.PrimvarsAPI(shape.GetPrim())
        widthsPv = primvarsAPI.CreatePrimvar(
            "widths", Sdf.ValueTypeNames.FloatArray)
        widthsPv.Set([1.0, 2.0])

        errors = validator.Validate(shape.GetPrim())
        self.assertTrue(len(errors) == 0)

        # both authored, primvars:widths wins — primvars has wrong count
        # widths attr matches, but primvar takes priority so should fail
        stage = Usd.Stage.CreateInMemory()
        shape = UsdGeom.Points.Define(stage, "/shape")
        UsdPhysics.CollisionAPI.Apply(shape.GetPrim())
        shape.GetPointsAttr().Set([Gf.Vec3f(1.0), Gf.Vec3f(2.0)])
        shape.GetWidthsAttr().Set([1.0, 2.0])
        primvarsAPI = UsdGeom.PrimvarsAPI(shape.GetPrim())
        widthsPv = primvarsAPI.CreatePrimvar(
            "widths", Sdf.ValueTypeNames.FloatArray)
        widthsPv.Set([1.0])

        errors = validator.Validate(shape.GetPrim())
        self.assertTrue(len(errors) == 1)
        self.assertTrue(errors[0].GetName() == "ColliderSpherePointsDataMissing")

        # neither authored — should fail
        stage = Usd.Stage.CreateInMemory()
        shape = UsdGeom.Points.Define(stage, "/shape")
        UsdPhysics.CollisionAPI.Apply(shape.GetPrim())
        shape.GetPointsAttr().Set([Gf.Vec3f(1.0)])

        errors = validator.Validate(shape.GetPrim())
        self.assertTrue(len(errors) == 1)
        self.assertTrue(errors[0].GetName() == "ColliderSpherePointsDataMissing")

        # indexed primvars:widths — 1 value, 2 indices matching 2 points
        stage = Usd.Stage.CreateInMemory()
        shape = UsdGeom.Points.Define(stage, "/shape")
        UsdPhysics.CollisionAPI.Apply(shape.GetPrim())
        shape.GetPointsAttr().Set([Gf.Vec3f(1.0), Gf.Vec3f(2.0)])
        primvarsAPI = UsdGeom.PrimvarsAPI(shape.GetPrim())
        widthsPv = primvarsAPI.CreatePrimvar(
            "widths", Sdf.ValueTypeNames.FloatArray)
        widthsPv.Set([1.0])
        widthsPv.SetIndices(Vt.IntArray([0, 0]))

        errors = validator.Validate(shape.GetPrim())
        self.assertTrue(len(errors) == 0)

        # indexed primvars:widths — wrong flattened count
        widthsPv.SetIndices(Vt.IntArray([0]))

        errors = validator.Validate(shape.GetPrim())
        self.assertTrue(len(errors) == 1)
        self.assertTrue(errors[0].GetName() == "ColliderSpherePointsDataMissing")

    def test_plane_collider_static_only(self):
        validationRegistry = UsdValidation.ValidationRegistry()
        validator = validationRegistry.GetOrLoadValidatorByName(
            "usdPhysicsValidators:ColliderChecker"
        )

        self.assertTrue(validator)

        stage = Usd.Stage.CreateInMemory()
        self.assertTrue(stage)

        # Static plane collider (no rigid body parent) - should pass
        plane = UsdGeom.Plane.Define(stage, "/staticPlane")
        UsdPhysics.CollisionAPI.Apply(plane.GetPrim())

        errors = validator.Validate(plane.GetPrim())
        self.assertTrue(len(errors) == 0)

        # Plane under a dynamic rigid body - should fail
        dynamicBody = UsdGeom.Xform.Define(stage, "/dynamicBody")
        rboAPI = UsdPhysics.RigidBodyAPI.Apply(dynamicBody.GetPrim())

        dynamicPlane = UsdGeom.Plane.Define(stage, "/dynamicBody/plane")
        UsdPhysics.CollisionAPI.Apply(dynamicPlane.GetPrim())

        errors = validator.Validate(dynamicPlane.GetPrim())
        self.assertTrue(len(errors) == 1)
        self.assertTrue(errors[0].GetName() == "ColliderPlaneDynamic")

        # Plane under a static rigid body (enabled=false) - should pass
        rboAPI.GetRigidBodyEnabledAttr().Set(False)

        errors = validator.Validate(dynamicPlane.GetPrim())
        self.assertTrue(len(errors) == 0)

        # Plane under a kinematic rigid body - should pass
        rboAPI.GetRigidBodyEnabledAttr().Set(True)
        rboAPI.GetKinematicEnabledAttr().Set(True)

        errors = validator.Validate(dynamicPlane.GetPrim())
        self.assertTrue(len(errors) == 0)

    def test_rigid_body_mass_api(self):
        validationRegistry = UsdValidation.ValidationRegistry()
        rigidBodyValidator = validationRegistry.GetOrLoadValidatorByName(
            "usdPhysicsValidators:RigidBodyChecker"
        )
        self.assertTrue(rigidBodyValidator)

        stage = Usd.Stage.CreateInMemory()
        self.assertTrue(stage)

        # negative mass should fail
        rigidbody = UsdGeom.Xform.Define(stage, "/rigidbody1")
        UsdPhysics.RigidBodyAPI.Apply(rigidbody.GetPrim())
        massAPI = UsdPhysics.MassAPI.Apply(rigidbody.GetPrim())
        massAPI.GetMassAttr().Set(-5.0)
        errors = rigidBodyValidator.Validate(rigidbody.GetPrim())
        self.assertEqual(len(errors), 1)
        self.assertEqual(errors[0].GetName(), "MassInvalidValues")

        # negative density should fail
        stage.RemovePrim(rigidbody.GetPrim().GetPrimPath())
        rigidbody = UsdGeom.Xform.Define(stage, "/rigidbody2")
        UsdPhysics.RigidBodyAPI.Apply(rigidbody.GetPrim())
        massAPI = UsdPhysics.MassAPI.Apply(rigidbody.GetPrim())
        massAPI.GetDensityAttr().Set(-10.0)
        errors = rigidBodyValidator.Validate(rigidbody.GetPrim())
        self.assertEqual(len(errors), 1)
        self.assertEqual(errors[0].GetName(), "DensityInvalidValues")

        # neither principalAxes nor diagonalInertia authored on rigid body - should pass
        rigidbody = UsdGeom.Xform.Define(stage, "/rigidBody3")
        UsdPhysics.RigidBodyAPI.Apply(rigidbody.GetPrim())
        UsdPhysics.MassAPI.Apply(rigidbody.GetPrim())
        errors = rigidBodyValidator.Validate(rigidbody.GetPrim())
        self.assertEqual(len(errors), 0)

        # only principalAxes authored with non-fallback value - should fail
        stage.RemovePrim(rigidbody.GetPrim().GetPrimPath())
        rigidbody = UsdGeom.Xform.Define(stage, "/rigidBody4")
        UsdPhysics.RigidBodyAPI.Apply(rigidbody.GetPrim())
        massAPI = UsdPhysics.MassAPI.Apply(rigidbody.GetPrim())
        massAPI.GetPrincipalAxesAttr().Set(Gf.Quatf(1.0, 0.0, 0.0, 0.0))
        errors = rigidBodyValidator.Validate(rigidbody.GetPrim())
        self.assertEqual(len(errors), 1)
        self.assertEqual(errors[0].GetName(), "InertiaInvalidValues")

        # only diagonalInertia authored with non-fallback value - should fail
        stage.RemovePrim(rigidbody.GetPrim().GetPrimPath())
        rigidbody = UsdGeom.Xform.Define(stage, "/rigidBody5")
        UsdPhysics.RigidBodyAPI.Apply(rigidbody.GetPrim())
        massAPI = UsdPhysics.MassAPI.Apply(rigidbody.GetPrim())
        massAPI.GetDiagonalInertiaAttr().Set(Gf.Vec3f(1.0, 2.0, 3.0))
        errors = rigidBodyValidator.Validate(rigidbody.GetPrim())
        self.assertEqual(len(errors), 1)
        self.assertEqual(errors[0].GetName(), "InertiaInvalidValues")

        # both authored with valid values - should pass
        stage.RemovePrim(rigidbody.GetPrim().GetPrimPath())
        rigidbody = UsdGeom.Xform.Define(stage, "/rigidBody6")
        UsdPhysics.RigidBodyAPI.Apply(rigidbody.GetPrim())
        massAPI = UsdPhysics.MassAPI.Apply(rigidbody.GetPrim())
        massAPI.GetPrincipalAxesAttr().Set(Gf.Quatf(1.0, 0.0, 0.0, 0.0))  # Valid unit quaternion
        massAPI.GetDiagonalInertiaAttr().Set(Gf.Vec3f(1.0, 2.0, 3.0))  # Valid positive values
        errors = rigidBodyValidator.Validate(rigidbody.GetPrim())
        self.assertEqual(len(errors), 0)

        # both authored with fallback values - should pass
        stage.RemovePrim(rigidbody.GetPrim().GetPrimPath())
        rigidbody = UsdGeom.Xform.Define(stage, "/rigidBody7")
        UsdPhysics.RigidBodyAPI.Apply(rigidbody.GetPrim())
        massAPI = UsdPhysics.MassAPI.Apply(rigidbody.GetPrim())
        massAPI.GetPrincipalAxesAttr().Set(Gf.Quatf(0.0, 0.0, 0.0, 0.0))  # Fallback value
        massAPI.GetDiagonalInertiaAttr().Set(Gf.Vec3f(0.0, 0.0, 0.0))  # Fallback value
        errors = rigidBodyValidator.Validate(rigidbody.GetPrim())
        self.assertEqual(len(errors), 0)

        # principalAxes fallback but diagonalInertia non-fallback - should fail
        stage.RemovePrim(rigidbody.GetPrim().GetPrimPath())
        rigidbody = UsdGeom.Xform.Define(stage, "/rigidBody8")
        UsdPhysics.RigidBodyAPI.Apply(rigidbody.GetPrim())
        massAPI = UsdPhysics.MassAPI.Apply(rigidbody.GetPrim())
        massAPI.GetPrincipalAxesAttr().Set(Gf.Quatf(0.0, 0.0, 0.0, 0.0))  # Fallback value
        massAPI.GetDiagonalInertiaAttr().Set(Gf.Vec3f(1.0, 2.0, 3.0))  # Non-fallback value
        errors = rigidBodyValidator.Validate(rigidbody.GetPrim())
        self.assertEqual(len(errors), 1)
        self.assertEqual(errors[0].GetName(), "InertiaInvalidValues")

        # principalAxes non-fallback but diagonalInertia fallback - should fail
        stage.RemovePrim(rigidbody.GetPrim().GetPrimPath())
        rigidbody = UsdGeom.Xform.Define(stage, "/rigidBody9")
        UsdPhysics.RigidBodyAPI.Apply(rigidbody.GetPrim())
        massAPI = UsdPhysics.MassAPI.Apply(rigidbody.GetPrim())
        massAPI.GetPrincipalAxesAttr().Set(Gf.Quatf(1.0, 0.0, 0.0, 0.0))  # Non-fallback value
        massAPI.GetDiagonalInertiaAttr().Set(Gf.Vec3f(0.0, 0.0, 0.0))  # Fallback value
        errors = rigidBodyValidator.Validate(rigidbody.GetPrim())
        self.assertEqual(len(errors), 1)
        self.assertEqual(errors[0].GetName(), "InertiaInvalidValues")

        # principalAxes is non-unit quaternion - should fail
        stage.RemovePrim(rigidbody.GetPrim().GetPrimPath())
        rigidbody = UsdGeom.Xform.Define(stage, "/rigidBody10")
        UsdPhysics.RigidBodyAPI.Apply(rigidbody.GetPrim())
        massAPI = UsdPhysics.MassAPI.Apply(rigidbody.GetPrim())
        massAPI.GetPrincipalAxesAttr().Set(Gf.Quatf(2.0, 0.0, 0.0, 0.0))  # Non-unit quaternion
        massAPI.GetDiagonalInertiaAttr().Set(Gf.Vec3f(1.0, 2.0, 3.0))
        errors = rigidBodyValidator.Validate(rigidbody.GetPrim())
        self.assertEqual(len(errors), 1)
        self.assertEqual(errors[0].GetName(), "InertiaInvalidValues")

        # diagonalInertia has negative values - should fail
        stage.RemovePrim(rigidbody.GetPrim().GetPrimPath())
        rigidbody = UsdGeom.Xform.Define(stage, "/rigidBody11")
        UsdPhysics.RigidBodyAPI.Apply(rigidbody.GetPrim())
        massAPI = UsdPhysics.MassAPI.Apply(rigidbody.GetPrim())
        massAPI.GetPrincipalAxesAttr().Set(Gf.Quatf(1.0, 0.0, 0.0, 0.0))
        massAPI.GetDiagonalInertiaAttr().Set(Gf.Vec3f(-1.0, 2.0, 3.0))  # Negative value
        errors = rigidBodyValidator.Validate(rigidbody.GetPrim())
        self.assertEqual(len(errors), 1)
        self.assertEqual(errors[0].GetName(), "InertiaInvalidValues")

        # diagonalInertia has zero values - should fail
        stage.RemovePrim(rigidbody.GetPrim().GetPrimPath())
        rigidbody = UsdGeom.Xform.Define(stage, "/rigidBody12")
        UsdPhysics.RigidBodyAPI.Apply(rigidbody.GetPrim())
        massAPI = UsdPhysics.MassAPI.Apply(rigidbody.GetPrim())
        massAPI.GetPrincipalAxesAttr().Set(Gf.Quatf(1.0, 0.0, 0.0, 0.0))
        massAPI.GetDiagonalInertiaAttr().Set(Gf.Vec3f(0.0, 2.0, 3.0))  # Zero value
        errors = rigidBodyValidator.Validate(rigidbody.GetPrim())
        self.assertEqual(len(errors), 1)
        self.assertEqual(errors[0].GetName(), "InertiaInvalidValues")

        # valid normalized quaternion on rigid body - should pass
        stage.RemovePrim(rigidbody.GetPrim().GetPrimPath())
        rigidbody = UsdGeom.Xform.Define(stage, "/rigidBody13")
        UsdPhysics.RigidBodyAPI.Apply(rigidbody.GetPrim())
        massAPI = UsdPhysics.MassAPI.Apply(rigidbody.GetPrim())
        # Quaternion representing 45 degree rotation around Y axis
        massAPI.GetPrincipalAxesAttr().Set(Gf.Quatf(0.9238795, 0.0, 0.3826834, 0.0))
        massAPI.GetDiagonalInertiaAttr().Set(Gf.Vec3f(1.0, 2.0, 3.0))
        errors = rigidBodyValidator.Validate(rigidbody.GetPrim())
        self.assertEqual(len(errors), 0)

    def test_collider_mass_api(self):
        validationRegistry = UsdValidation.ValidationRegistry()
        colliderValidator = validationRegistry.GetOrLoadValidatorByName(
            "usdPhysicsValidators:ColliderChecker"
        )
        self.assertTrue(colliderValidator)

        stage = Usd.Stage.CreateInMemory()
        self.assertTrue(stage)

        # negative mass should fail
        collider = UsdGeom.Cylinder.Define(stage, "/collider1")
        UsdPhysics.CollisionAPI.Apply(collider.GetPrim())
        massAPI = UsdPhysics.MassAPI.Apply(collider.GetPrim())
        massAPI.GetMassAttr().Set(-5.0)
        errors = colliderValidator.Validate(collider.GetPrim())
        self.assertEqual(len(errors), 1)
        self.assertEqual(errors[0].GetName(), "MassInvalidValues")

        # negative density should fail
        stage.RemovePrim(collider.GetPrim().GetPrimPath())
        collider = UsdGeom.Cone.Define(stage, "/collider2")
        UsdPhysics.CollisionAPI.Apply(collider.GetPrim())
        massAPI = UsdPhysics.MassAPI.Apply(collider.GetPrim())
        massAPI.GetDensityAttr().Set(-10.0)
        errors = colliderValidator.Validate(collider.GetPrim())
        self.assertEqual(len(errors), 1)
        self.assertEqual(errors[0].GetName(), "DensityInvalidValues")

        # neither authored - should pass
        collider = UsdGeom.Sphere.Define(stage, "/collider3")
        UsdPhysics.CollisionAPI.Apply(collider.GetPrim())
        UsdPhysics.MassAPI.Apply(collider.GetPrim())
        errors = colliderValidator.Validate(collider.GetPrim())
        self.assertEqual(len(errors), 0)

        # only principalAxes authored - should fail
        stage.RemovePrim(collider.GetPrim().GetPrimPath())
        collider = UsdGeom.Cube.Define(stage, "/collider4")
        UsdPhysics.CollisionAPI.Apply(collider.GetPrim())
        massAPI = UsdPhysics.MassAPI.Apply(collider.GetPrim())
        massAPI.GetPrincipalAxesAttr().Set(Gf.Quatf(1.0, 0.0, 0.0, 0.0))
        errors = colliderValidator.Validate(collider.GetPrim())
        self.assertEqual(len(errors), 1)
        self.assertEqual(errors[0].GetName(), "InertiaInvalidValues")

        # both authored with valid values - should pass
        stage.RemovePrim(collider.GetPrim().GetPrimPath())
        collider = UsdGeom.Mesh.Define(stage, "/collider5")
        UsdPhysics.CollisionAPI.Apply(collider.GetPrim())
        massAPI = UsdPhysics.MassAPI.Apply(collider.GetPrim())
        massAPI.GetPrincipalAxesAttr().Set(Gf.Quatf(1.0, 0.0, 0.0, 0.0))
        massAPI.GetDiagonalInertiaAttr().Set(Gf.Vec3f(1.0, 2.0, 3.0))
        errors = colliderValidator.Validate(collider.GetPrim())
        self.assertEqual(len(errors), 0)

        # with invalid inertia - should fail
        stage.RemovePrim(collider.GetPrim().GetPrimPath())
        collider = UsdGeom.Capsule.Define(stage, "/collider6")
        UsdPhysics.CollisionAPI.Apply(collider.GetPrim())
        massAPI = UsdPhysics.MassAPI.Apply(collider.GetPrim())
        massAPI.GetPrincipalAxesAttr().Set(Gf.Quatf(2.0, 0.0, 0.0, 0.0))  # Non-unit quaternion
        massAPI.GetDiagonalInertiaAttr().Set(Gf.Vec3f(1.0, 2.0, 3.0))
        errors = colliderValidator.Validate(collider.GetPrim())
        self.assertEqual(len(errors), 1)
        self.assertEqual(errors[0].GetName(), "InertiaInvalidValues")


if __name__ == "__main__":
    unittest.main()
