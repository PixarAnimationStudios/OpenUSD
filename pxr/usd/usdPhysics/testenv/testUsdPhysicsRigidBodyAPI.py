#!/pxrpythonsubst
#
# Copyright 2021 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

from __future__ import print_function

import sys, os, unittest
from pxr import Tf, Usd, Sdf, UsdGeom, UsdShade, Gf, UsdPhysics

class TestUsdPhysicsRigidBodyAPI(unittest.TestCase):

    def setup_scene(self, metersPerUnit = 1.0, kilogramsPerUnit = 1.0):
        stage = Usd.Stage.CreateInMemory()
        self.assertTrue(stage)
        self.stage = stage

        # setup stage units
        UsdGeom.SetStageUpAxis(stage, "Z")
        UsdGeom.SetStageMetersPerUnit(stage, metersPerUnit)
        UsdPhysics.SetStageKilogramsPerUnit(stage, kilogramsPerUnit)

    def get_collision_shape_local_transfrom(self, collisionLocalToWorld, bodyLocalToWorld):
        mat = collisionLocalToWorld *  bodyLocalToWorld.GetInverse()
        colLocalTransform = Gf.Transform(mat)

        localPos = Gf.Vec3f(0.0)
        localRotOut = Gf.Quatf(1.0)

        localPos = colLocalTransform.GetTranslation()
        localRotOut = colLocalTransform.GetRotation().GetQuat()

        # now apply the body scale to localPos
        # physics does not support scales, so a rigid body scale has to be baked into the localPos    
        tr = Gf.Transform(bodyLocalToWorld)
        sc = tr.GetScale()

        localPos[0] = localPos[0] * sc[0]
        localPos[1] = localPos[1] * sc[1]
        localPos[2] = localPos[2] * sc[2]

        return localPos, localRotOut


    def mass_information_fn(self, prim):
        massInfo = UsdPhysics.RigidBodyAPI.MassInformation()
        if prim.IsA(UsdGeom.Cube):
            cubeLocalToWorldTransform = UsdGeom.Xformable(prim).ComputeLocalToWorldTransform(Usd.TimeCode.Default())
            extents = Gf.Transform(cubeLocalToWorldTransform).GetScale()

            cube = UsdGeom.Cube(prim)
            sizeAttr = cube.GetSizeAttr().Get()
            sizeAttr = abs(sizeAttr)
            extents = extents * sizeAttr

            # cube volume
            massInfo.volume = extents[0] * extents[1] * extents[2]

            # cube inertia
            inertia_diagonal = Gf.Vec3f(1.0/12.0 * ( extents[1]*extents[1] + extents[2]*extents[2]), 1.0/12.0 * ( extents[0]*extents[0] + extents[2]*extents[2]), 1.0/12.0 * ( extents[0]*extents[0] + extents[1]*extents[1]))
            massInfo.inertia = Gf.Matrix3f(1.0)
            massInfo.inertia.SetDiagonal(inertia_diagonal)

            # CoM
            massInfo.centerOfMass = Gf.Vec3f(0.0)

            # local pose
            if prim == self.rigidBodyPrim:
                massInfo.localPos = Gf.Vec3f(0.0)
                massInfo.localRot = Gf.Quatf(1.0)
            else:
                # massInfo.localPos, massInfo.localRot
                lp, lr = self.get_collision_shape_local_transfrom(cubeLocalToWorldTransform, self.rigidBodyWorldTransform)  
                massInfo.localPos = Gf.Vec3f(lp)
                massInfo.localRot = Gf.Quatf(lr)

        else:
            print("UsdGeom type not supported.")
            massInfo.volume = -1.0

        return massInfo

    def compare_mass_information(self, rigidBodyAPI, expectedMass, expectedInertia = None, expectedCoM = None, expectedPrincipalAxes = None):
        mass, inertia, centerOfMass, principalAxes = rigidBodyAPI.ComputeMassProperties(self.mass_information_fn)

        toleranceEpsilon = 0.01

        self.assertTrue(abs(mass - expectedMass) < toleranceEpsilon)

        if expectedCoM is not None:
            self.assertTrue(Gf.IsClose(centerOfMass, expectedCoM, toleranceEpsilon))

        if expectedInertia is not None:
            self.assertTrue(Gf.IsClose(inertia, expectedInertia, toleranceEpsilon))

        if expectedPrincipalAxes is not None:
            self.assertTrue(Gf.IsClose(principalAxes.GetImaginary(), expectedPrincipalAxes.GetImaginary(), toleranceEpsilon))
            self.assertTrue(abs(principalAxes.GetReal() - expectedPrincipalAxes.GetReal()) < toleranceEpsilon)


    # base cube rigid body, expect CUBE rigid body mass properties computed from default density 1000
    def test_mass_rigid_body_cube(self):
        self.setup_scene()

        # Create test collider cube    
        self.cube = UsdGeom.Cube.Define(self.stage, "/cube")
        self.cube.GetSizeAttr().Set(1.0)
        UsdPhysics.CollisionAPI.Apply(self.cube.GetPrim())
        rigidBodyAPI = UsdPhysics.RigidBodyAPI.Apply(self.cube.GetPrim())

        self.rigidBodyWorldTransform = UsdGeom.Xformable(self.cube.GetPrim()).ComputeLocalToWorldTransform(Usd.TimeCode.Default())
        self.rigidBodyPrim = self.cube.GetPrim()
                        
        self.compare_mass_information(rigidBodyAPI, 1000.0, expectedCoM=Gf.Vec3f(0.0), expectedInertia=Gf.Vec3f(166.667))

    def test_mass_blocked_attributes(self):
        self.setup_scene()
        body = UsdGeom.Xform.Define(self.stage, "/body")
        body.AddScaleOp().Set(Gf.Vec3f(2.0))
        rigidBodyAPI = UsdPhysics.RigidBodyAPI.Apply(body.GetPrim())
        bodyMassAPI = UsdPhysics.MassAPI.Apply(body.GetPrim())
        cube = UsdGeom.Cube.Define(self.stage, "/body/cube")
        cube.GetSizeAttr().Set(1.0)
        cube.AddTranslateOp().Set(Gf.Vec3f(0.5, -0.25, 0.75))
        cube.AddRotateZOp().Set(30.0)
        cube.AddScaleOp().Set(Gf.Vec3f(0.2, 0.3, 0.4))
        UsdPhysics.CollisionAPI.Apply(cube.GetPrim())
        colliderMassAPI = UsdPhysics.MassAPI.Apply(cube.GetPrim())

        # The scaled box has sides (0.4, 0.6, 0.8), volume 0.192,
        # and unit-density inertia V/12 * (y*y+z*z, x*x+z*z, x*x+y*y).
        volume = 0.192
        unitInertia = Gf.Matrix3f(0.016, 0, 0,
                                0, 0.0128, 0, 0, 0, 0.00832)
        position = Gf.Vec3f(1.0, -0.5, 1.5)
        rotation = Gf.Quatf(Gf.Rotation(Gf.Vec3d(0, 0, 1), 30).GetQuat())
        rotationMatrix = Gf.Matrix3f(rotation)

        def mass_info(prim):
            self.assertEqual(prim, cube.GetPrim())
            info = UsdPhysics.RigidBodyAPI.MassInformation()
            info.volume = volume
            info.inertia = unitInertia
            info.centerOfMass = Gf.Vec3f(0.0)
            info.localPos = position
            info.localRot = rotation
            return info

        def check(expectedMass, expectedInertia, expectedCoM):
            mass, diagonal, com, axes = rigidBodyAPI.ComputeMassProperties(
                mass_info)
            self.assertAlmostEqual(mass, expectedMass, delta=0.0001)
            self.assertTrue(Gf.IsClose(com, expectedCoM, 0.0001))
            # Compare the represented tensor; equivalent principal-axis
            # permutations and quaternion signs are equally valid.
            matrix = Gf.Matrix3f(axes)
            tensor = matrix.GetTranspose() * Gf.Matrix3f(diagonal) * matrix
            self.assertTrue(Gf.IsClose(tensor, expectedInertia, 0.0001))

        computedInertia = (rotationMatrix.GetTranspose() * unitInertia *
                           rotationMatrix * 1000.0)
        names = ("mass", "diagonalInertia", "principalAxes", "centerOfMass")
        authoredAxes = Gf.Quatf(
            Gf.Rotation(Gf.Vec3d(1, 0, 0), 40).GetQuat())
        authoredValues = (7.0, Gf.Vec3f(0.7, 1.1, 1.3), authoredAxes,
                          Gf.Vec3f(0.1, 0.2, 0.3))
        authoredMatrix = Gf.Matrix3f(authoredAxes)
        authoredInertia = (authoredMatrix.GetTranspose() *
                           Gf.Matrix3f(authoredValues[1]) * authoredMatrix)

        for massAPI in (bodyMassAPI, colliderMassAPI):
            prim = massAPI.GetPrim()
            attrs = [prim.GetAttribute("physics:" + name) for name in names]
            fallbacks = [attr.Get() for attr in attrs]
            self.assertEqual(fallbacks[:3],
                             [0.0, Gf.Vec3f(0.0), Gf.Quatf(0.0)])
            self.assertEqual(fallbacks[3], Gf.Vec3f(float("-inf")))
            with self.subTest(prim=prim.GetPath(), attributes="unauthored"):
                check(192.0, computedInertia, position)

            for blocked in [(attr,) for attr in attrs] + [tuple(attrs)]:
                with self.subTest(prim=prim.GetPath(),
                                  attributes=[a.GetName() for a in blocked]):
                    for attr in blocked:
                        attr.Block()
                        self.assertIsNone(attr.Get())
                    check(192.0, computedInertia, position)
                for attr in blocked:
                    attr.Clear()

            with self.subTest(prim=prim.GetPath(), attributes="authored"):
                for attr, value in zip(attrs, authoredValues):
                    attr.Set(value)
                if prim == body.GetPrim():
                    check(7.0, authoredInertia, Gf.Vec3f(0.2, 0.4, 0.6))
                else:
                    # Collider CoM is scaled, rotated and translated into
                    # body space; authored inertia is rotated into body space.
                    check(7.0, rotationMatrix.GetTranspose() *
                          authoredInertia * rotationMatrix,
                          position + rotation.Transform(
                              Gf.Vec3f(0.04, 0.12, 0.24)))
            for attr in attrs:
                attr.Clear()

        # Block all four attributes on both prims while exercising density
        # precedence: collider, body, material, then the default density.
        for prim in (body.GetPrim(), cube.GetPrim()):
            for name in names:
                prim.GetAttribute("physics:" + name).Block()
        material = UsdShade.Material.Define(self.stage, "/material")
        materialAPI = UsdPhysics.MaterialAPI.Apply(material.GetPrim())
        UsdShade.MaterialBindingAPI.Apply(cube.GetPrim()).Bind(
            material, UsdShade.Tokens.weakerThanDescendants, "physics")
        densities = [colliderMassAPI.GetDensityAttr(),
                     bodyMassAPI.GetDensityAttr(), materialAPI.GetDensityAttr()]
        for attr, density in zip(densities, (300.0, 200.0, 100.0)):
            attr.Set(density)
        for density, attr in zip((300.0, 200.0, 100.0, 1000.0),
                                 densities + [None]):
            with self.subTest(density=density):
                check(volume * density, computedInertia * (density / 1000.0),
                      position)
            if attr:
                attr.Block()

    # density tests

    # density test, applied density to a body
    def test_mass_rigid_body_cube_rigid_body_density(self):
        self.setup_scene()

        # top level xform - rigid body
        self.xform = UsdGeom.Xform.Define(self.stage, "/xform")
        rigidBodyAPI = UsdPhysics.RigidBodyAPI.Apply(self.xform.GetPrim())
        massAPI = UsdPhysics.MassAPI.Apply(self.xform.GetPrim())

        # set half the default gravity
        massAPI.GetDensityAttr().Set(500.0)

        # Create test collider cube    
        self.cube = UsdGeom.Cube.Define(self.stage, "/xform/cube")
        self.cube.GetSizeAttr().Set(1.0)
        UsdPhysics.CollisionAPI.Apply(self.cube.GetPrim())

        self.rigidBodyWorldTransform = UsdGeom.Xformable(self.xform.GetPrim()).ComputeLocalToWorldTransform(Usd.TimeCode.Default())
        self.rigidBodyPrim = self.xform.GetPrim()
                        
        self.compare_mass_information(rigidBodyAPI, 500.0, expectedCoM=Gf.Vec3f(0.0), expectedInertia=Gf.Vec3f(166.667*0.5))

    # density test, applied density to a collider
    def test_mass_rigid_body_cube_collider_density(self):
        self.setup_scene()

        # top level xform - rigid body
        self.xform = UsdGeom.Xform.Define(self.stage, "/xform")
        rigidBodyAPI = UsdPhysics.RigidBodyAPI.Apply(self.xform.GetPrim())        

        # Create test collider cube    
        self.cube = UsdGeom.Cube.Define(self.stage, "/xform/cube")
        self.cube.GetSizeAttr().Set(1.0)
        UsdPhysics.CollisionAPI.Apply(self.cube.GetPrim())
        massAPI = UsdPhysics.MassAPI.Apply(self.cube.GetPrim())

        # set half the default gravity
        massAPI.GetDensityAttr().Set(500.0)

        self.rigidBodyWorldTransform = UsdGeom.Xformable(self.xform.GetPrim()).ComputeLocalToWorldTransform(Usd.TimeCode.Default())
        self.rigidBodyPrim = self.xform.GetPrim()
                        
        self.compare_mass_information(rigidBodyAPI, 500.0, expectedCoM=Gf.Vec3f(0.0), expectedInertia=Gf.Vec3f(166.667*0.5))

    # density test, applied density to a material
    def test_mass_rigid_body_cube_material_density(self):
        self.setup_scene()

        # top level xform - rigid body
        self.xform = UsdGeom.Xform.Define(self.stage, "/xform")
        rigidBodyAPI = UsdPhysics.RigidBodyAPI.Apply(self.xform.GetPrim())        

        # Create test collider cube    
        self.cube = UsdGeom.Cube.Define(self.stage, "/xform/cube")
        self.cube.GetSizeAttr().Set(1.0)
        UsdPhysics.CollisionAPI.Apply(self.cube.GetPrim())
        
        # Create base physics material
        self.basePhysicsMaterial = UsdShade.Material.Define(self.stage, "/basePhysicsMaterial")
        materialAPI = UsdPhysics.MaterialAPI.Apply(self.basePhysicsMaterial.GetPrim())
        # set half the default gravity
        materialAPI.GetDensityAttr().Set(500.0)
        bindingAPI = UsdShade.MaterialBindingAPI.Apply(self.cube.GetPrim())        
        bindingAPI.Bind(self.basePhysicsMaterial, UsdShade.Tokens.weakerThanDescendants, "physics")


        self.rigidBodyWorldTransform = UsdGeom.Xformable(self.xform.GetPrim()).ComputeLocalToWorldTransform(Usd.TimeCode.Default())
        self.rigidBodyPrim = self.xform.GetPrim()
                        
        self.compare_mass_information(rigidBodyAPI, 500.0, expectedCoM=Gf.Vec3f(0.0), expectedInertia=Gf.Vec3f(166.667*0.5))

    # density test, collision density precedence
    def test_mass_rigid_body_cube_density_precedence(self):
        self.setup_scene()

        # top level xform - rigid body
        self.xform = UsdGeom.Xform.Define(self.stage, "/xform")
        rigidBodyAPI = UsdPhysics.RigidBodyAPI.Apply(self.xform.GetPrim())        
        massAPI = UsdPhysics.MassAPI.Apply(self.xform.GetPrim())
        massAPI.GetDensityAttr().Set(5000.0)

        # Create test collider cube    
        self.cube = UsdGeom.Cube.Define(self.stage, "/xform/cube")
        self.cube.GetSizeAttr().Set(1.0)
        UsdPhysics.CollisionAPI.Apply(self.cube.GetPrim())
        massAPI = UsdPhysics.MassAPI.Apply(self.cube.GetPrim())

        # collision density does have precedence
        massAPI.GetDensityAttr().Set(500.0)
        
        # Create base physics material
        self.basePhysicsMaterial = UsdShade.Material.Define(self.stage, "/basePhysicsMaterial")
        materialAPI = UsdPhysics.MaterialAPI.Apply(self.basePhysicsMaterial.GetPrim())
        # set half the default gravity
        materialAPI.GetDensityAttr().Set(2000.0)
        bindingAPI = UsdShade.MaterialBindingAPI.Apply(self.cube.GetPrim())        
        bindingAPI.Bind(self.basePhysicsMaterial, UsdShade.Tokens.weakerThanDescendants, "physics")


        self.rigidBodyWorldTransform = UsdGeom.Xformable(self.xform.GetPrim()).ComputeLocalToWorldTransform(Usd.TimeCode.Default())
        self.rigidBodyPrim = self.xform.GetPrim()
                        
        self.compare_mass_information(rigidBodyAPI, 500.0, expectedCoM=Gf.Vec3f(0.0), expectedInertia=Gf.Vec3f(166.667*0.5))

    # mass tests

    # mass test, applied mass to a body
    def test_mass_rigid_body_cube_rigid_body_mass(self):
        self.setup_scene()

        # top level xform - rigid body
        self.xform = UsdGeom.Xform.Define(self.stage, "/xform")
        rigidBodyAPI = UsdPhysics.RigidBodyAPI.Apply(self.xform.GetPrim())
        massAPI = UsdPhysics.MassAPI.Apply(self.xform.GetPrim())

        # increase the mass twice
        massAPI.GetMassAttr().Set(2000.0)

        # Create test collider cube    
        self.cube = UsdGeom.Cube.Define(self.stage, "/xform/cube")
        self.cube.GetSizeAttr().Set(1.0)
        UsdPhysics.CollisionAPI.Apply(self.cube.GetPrim())

        self.rigidBodyWorldTransform = UsdGeom.Xformable(self.xform.GetPrim()).ComputeLocalToWorldTransform(Usd.TimeCode.Default())
        self.rigidBodyPrim = self.xform.GetPrim()
                        
        self.compare_mass_information(rigidBodyAPI, 2000.0, expectedCoM=Gf.Vec3f(0.0), expectedInertia=Gf.Vec3f(166.667*2.0))

    # mass test, applied mass to a collider
    def test_mass_rigid_body_cube_collider_mass(self):
        self.setup_scene()

        # top level xform - rigid body
        self.xform = UsdGeom.Xform.Define(self.stage, "/xform")
        rigidBodyAPI = UsdPhysics.RigidBodyAPI.Apply(self.xform.GetPrim())        

        # Create test collider cube    
        self.cube = UsdGeom.Cube.Define(self.stage, "/xform/cube")
        self.cube.GetSizeAttr().Set(1.0)
        UsdPhysics.CollisionAPI.Apply(self.cube.GetPrim())
        massAPI = UsdPhysics.MassAPI.Apply(self.cube.GetPrim())

        # increase the mass twice
        massAPI.GetMassAttr().Set(2000.0)

        self.rigidBodyWorldTransform = UsdGeom.Xformable(self.xform.GetPrim()).ComputeLocalToWorldTransform(Usd.TimeCode.Default())
        self.rigidBodyPrim = self.xform.GetPrim()
                        
        self.compare_mass_information(rigidBodyAPI, 2000.0, expectedCoM=Gf.Vec3f(0.0), expectedInertia=Gf.Vec3f(166.667*2.0))

    # mass test, applied mass precedence on a body
    def test_mass_rigid_body_cube_mass_precedence(self):
        self.setup_scene()

        # top level xform - rigid body
        self.xform = UsdGeom.Xform.Define(self.stage, "/xform")
        rigidBodyAPI = UsdPhysics.RigidBodyAPI.Apply(self.xform.GetPrim())        
        massAPI = UsdPhysics.MassAPI.Apply(self.xform.GetPrim())

        # increase the mass twice (has a precedence)
        massAPI.GetMassAttr().Set(2000.0)

        # Create test collider cube    
        self.cube = UsdGeom.Cube.Define(self.stage, "/xform/cube")
        self.cube.GetSizeAttr().Set(1.0)
        UsdPhysics.CollisionAPI.Apply(self.cube.GetPrim())
        massAPI = UsdPhysics.MassAPI.Apply(self.cube.GetPrim())

        # increase the mass twice
        massAPI.GetMassAttr().Set(500.0)

        self.rigidBodyWorldTransform = UsdGeom.Xformable(self.xform.GetPrim()).ComputeLocalToWorldTransform(Usd.TimeCode.Default())
        self.rigidBodyPrim = self.xform.GetPrim()
                        
        self.compare_mass_information(rigidBodyAPI, 2000.0, expectedCoM=Gf.Vec3f(0.0), expectedInertia=Gf.Vec3f(166.667*2.0))


    # CoM tests
    # com test, applied com to a body
    def test_mass_rigid_body_cube_rigid_body_com(self):
        self.setup_scene()

        # top level xform - rigid body
        self.xform = UsdGeom.Xform.Define(self.stage, "/xform")
        rigidBodyAPI = UsdPhysics.RigidBodyAPI.Apply(self.xform.GetPrim())
        massAPI = UsdPhysics.MassAPI.Apply(self.xform.GetPrim())

        # custom CoM
        massAPI.GetCenterOfMassAttr().Set(Gf.Vec3f(2.0))

        # Create test collider cube    
        self.cube = UsdGeom.Cube.Define(self.stage, "/xform/cube")
        self.cube.GetSizeAttr().Set(1.0)
        UsdPhysics.CollisionAPI.Apply(self.cube.GetPrim())

        self.rigidBodyWorldTransform = UsdGeom.Xformable(self.xform.GetPrim()).ComputeLocalToWorldTransform(Usd.TimeCode.Default())
        self.rigidBodyPrim = self.xform.GetPrim()
                        
        self.compare_mass_information(rigidBodyAPI, 1000.0, expectedCoM=Gf.Vec3f(2.0))

    # com test, applied com to a collider
    def test_mass_rigid_body_cube_collider_com(self):
        self.setup_scene()

        # top level xform - rigid body
        self.xform = UsdGeom.Xform.Define(self.stage, "/xform")
        rigidBodyAPI = UsdPhysics.RigidBodyAPI.Apply(self.xform.GetPrim())        

        # Create test collider cube    
        self.cube = UsdGeom.Cube.Define(self.stage, "/xform/cube")
        self.cube.GetSizeAttr().Set(1.0)
        UsdPhysics.CollisionAPI.Apply(self.cube.GetPrim())
        massAPI = UsdPhysics.MassAPI.Apply(self.cube.GetPrim())

        # custom CoM
        massAPI.GetCenterOfMassAttr().Set(Gf.Vec3f(2.0))

        self.rigidBodyWorldTransform = UsdGeom.Xformable(self.xform.GetPrim()).ComputeLocalToWorldTransform(Usd.TimeCode.Default())
        self.rigidBodyPrim = self.xform.GetPrim()
                        
        self.compare_mass_information(rigidBodyAPI, 1000.0, expectedCoM=Gf.Vec3f(2.0))

    # com test, precedence test rigid body has a precedence
    def test_mass_rigid_body_cube_com_precedence(self):
        self.setup_scene()

        # top level xform - rigid body
        self.xform = UsdGeom.Xform.Define(self.stage, "/xform")
        rigidBodyAPI = UsdPhysics.RigidBodyAPI.Apply(self.xform.GetPrim())        
        massAPI = UsdPhysics.MassAPI.Apply(self.xform.GetPrim())

        # custom CoM (has precedence)
        massAPI.GetCenterOfMassAttr().Set(Gf.Vec3f(2.0))

        # Create test collider cube    
        self.cube = UsdGeom.Cube.Define(self.stage, "/xform/cube")
        self.cube.GetSizeAttr().Set(1.0)
        UsdPhysics.CollisionAPI.Apply(self.cube.GetPrim())
        massAPI = UsdPhysics.MassAPI.Apply(self.cube.GetPrim())

        # custom CoM
        massAPI.GetCenterOfMassAttr().Set(Gf.Vec3f(1.0))

        self.rigidBodyWorldTransform = UsdGeom.Xformable(self.xform.GetPrim()).ComputeLocalToWorldTransform(Usd.TimeCode.Default())
        self.rigidBodyPrim = self.xform.GetPrim()
                        
        self.compare_mass_information(rigidBodyAPI, 1000.0, expectedCoM=Gf.Vec3f(2.0))

    # inertia tests
    # inertia test, applied inertia to a body
    def test_mass_rigid_body_cube_rigid_body_inertia(self):
        self.setup_scene()

        # top level xform - rigid body
        self.xform = UsdGeom.Xform.Define(self.stage, "/xform")
        rigidBodyAPI = UsdPhysics.RigidBodyAPI.Apply(self.xform.GetPrim())
        massAPI = UsdPhysics.MassAPI.Apply(self.xform.GetPrim())

        # custom inertia
        massAPI.GetDiagonalInertiaAttr().Set(Gf.Vec3f(2.0))

        # Create test collider cube    
        self.cube = UsdGeom.Cube.Define(self.stage, "/xform/cube")
        self.cube.GetSizeAttr().Set(1.0)
        UsdPhysics.CollisionAPI.Apply(self.cube.GetPrim())

        self.rigidBodyWorldTransform = UsdGeom.Xformable(self.xform.GetPrim()).ComputeLocalToWorldTransform(Usd.TimeCode.Default())
        self.rigidBodyPrim = self.xform.GetPrim()
                        
        self.compare_mass_information(rigidBodyAPI, 1000.0, expectedInertia=Gf.Vec3f(2.0))

    # com test, applied com to a collider
    def test_mass_rigid_body_cube_collider_inertia(self):
        self.setup_scene()

        # top level xform - rigid body
        self.xform = UsdGeom.Xform.Define(self.stage, "/xform")
        rigidBodyAPI = UsdPhysics.RigidBodyAPI.Apply(self.xform.GetPrim())        

        # Create test collider cube    
        self.cube = UsdGeom.Cube.Define(self.stage, "/xform/cube")
        self.cube.GetSizeAttr().Set(1.0)
        UsdPhysics.CollisionAPI.Apply(self.cube.GetPrim())
        massAPI = UsdPhysics.MassAPI.Apply(self.cube.GetPrim())

        # custom inertia
        massAPI.GetDiagonalInertiaAttr().Set(Gf.Vec3f(2.0))

        self.rigidBodyWorldTransform = UsdGeom.Xformable(self.xform.GetPrim()).ComputeLocalToWorldTransform(Usd.TimeCode.Default())
        self.rigidBodyPrim = self.xform.GetPrim()
                        
        self.compare_mass_information(rigidBodyAPI, 1000.0, expectedInertia=Gf.Vec3f(2.0))


    # inertia test, precedence test rigid body has a precedence
    def test_mass_rigid_body_cube_inertia_precedence(self):
        self.setup_scene()

        # top level xform - rigid body
        self.xform = UsdGeom.Xform.Define(self.stage, "/xform")
        rigidBodyAPI = UsdPhysics.RigidBodyAPI.Apply(self.xform.GetPrim())        
        massAPI = UsdPhysics.MassAPI.Apply(self.xform.GetPrim())

        # custom inertia (has precedence)
        massAPI.GetDiagonalInertiaAttr().Set(Gf.Vec3f(2.0))

        # Create test collider cube    
        self.cube = UsdGeom.Cube.Define(self.stage, "/xform/cube")
        self.cube.GetSizeAttr().Set(1.0)
        UsdPhysics.CollisionAPI.Apply(self.cube.GetPrim())
        massAPI = UsdPhysics.MassAPI.Apply(self.cube.GetPrim())

        # custom inertia
        massAPI.GetDiagonalInertiaAttr().Set(Gf.Vec3f(1.0))

        self.rigidBodyWorldTransform = UsdGeom.Xformable(self.xform.GetPrim()).ComputeLocalToWorldTransform(Usd.TimeCode.Default())
        self.rigidBodyPrim = self.xform.GetPrim()
                        
        self.compare_mass_information(rigidBodyAPI, 1000.0, expectedInertia=Gf.Vec3f(2.0))

    # inertia test, test compound body
    def test_mass_rigid_body_cube_rigid_body_compound(self):
        self.setup_scene()

        # top level xform - rigid body
        self.xform = UsdGeom.Xform.Define(self.stage, "/xform")
        rigidBodyAPI = UsdPhysics.RigidBodyAPI.Apply(self.xform.GetPrim())        

        size = 1.0
        scale = Gf.Vec3f(3.0, 2.0, 3.0)        

        # Create test collider cube0
        cube = UsdGeom.Cube.Define(self.stage, "/xform/cube0")
        cube.CreateSizeAttr(size)
        cube.AddTranslateOp().Set(Gf.Vec3f(100.0, 20.0, 10.0))
        cube0RotateOp = cube.AddRotateXYZOp()
        cube0RotateOp.Set(Gf.Vec3f(0.0,0.0,45.0))
        cube.AddScaleOp().Set(scale)
        UsdPhysics.CollisionAPI.Apply(cube.GetPrim())

        # Create test collider cube1
        cube = UsdGeom.Cube.Define(self.stage, "/xform/cube1")
        cube.CreateSizeAttr(size)
        cube.AddTranslateOp().Set(Gf.Vec3f(-100.0, 20.0, 10.0))
        cube1RotateOp = cube.AddRotateXYZOp()
        cube1RotateOp.Set(Gf.Vec3f(0.0,0.0,45.0))
        cube.AddScaleOp().Set(scale)
        UsdPhysics.CollisionAPI.Apply(cube.GetPrim())

        self.rigidBodyWorldTransform = UsdGeom.Xformable(self.xform.GetPrim()).ComputeLocalToWorldTransform(Usd.TimeCode.Default())
        self.rigidBodyPrim = self.xform.GetPrim()
                        
        mass, inertia_compare, centerOfMass, principalAxes = rigidBodyAPI.ComputeMassProperties(self.mass_information_fn)

        cube0RotateOp.Set(Gf.Vec3f(0.0,90.0,45.0))
        cube1RotateOp.Set(Gf.Vec3f(0.0,0.0,45.0))

        mass, inertia, centerOfMass, principalAxes = rigidBodyAPI.ComputeMassProperties(self.mass_information_fn)

        toleranceEpsilon = 1
        self.assertTrue(Gf.IsClose(inertia, inertia_compare, toleranceEpsilon))


    # principal axes tests
    # principal axes test, applied principal axis to a body
    def test_mass_rigid_body_cube_rigid_body_principal_axes(self):
        self.setup_scene()

        # top level xform - rigid body
        self.xform = UsdGeom.Xform.Define(self.stage, "/xform")
        rigidBodyAPI = UsdPhysics.RigidBodyAPI.Apply(self.xform.GetPrim())
        massAPI = UsdPhysics.MassAPI.Apply(self.xform.GetPrim())

        # custom principal axes
        massAPI.GetPrincipalAxesAttr().Set(Gf.Quatf(0.707, 0.0, 0.707, 0.0))

        # Create test collider cube    
        self.cube = UsdGeom.Cube.Define(self.stage, "/xform/cube")
        self.cube.GetSizeAttr().Set(1.0)
        UsdPhysics.CollisionAPI.Apply(self.cube.GetPrim())

        self.rigidBodyWorldTransform = UsdGeom.Xformable(self.xform.GetPrim()).ComputeLocalToWorldTransform(Usd.TimeCode.Default())
        self.rigidBodyPrim = self.xform.GetPrim()
                        
        self.compare_mass_information(rigidBodyAPI, 1000.0, expectedPrincipalAxes=Gf.Quatf(0.707, 0.0, 0.707, 0.0))

    # metersPerUnit default change
    def test_mass_rigid_body_cube_cm_units(self):
        self.setup_scene(metersPerUnit = 0.01)

        # Create test collider cube    
        self.cube = UsdGeom.Cube.Define(self.stage, "/cube")
        self.cube.GetSizeAttr().Set(1.0)
        UsdPhysics.CollisionAPI.Apply(self.cube.GetPrim())
        rigidBodyAPI = UsdPhysics.RigidBodyAPI.Apply(self.cube.GetPrim())

        self.rigidBodyWorldTransform = UsdGeom.Xformable(self.cube.GetPrim()).ComputeLocalToWorldTransform(Usd.TimeCode.Default())
        self.rigidBodyPrim = self.cube.GetPrim()
                        
        massScale = 0.01 * 0.01 * 0.01
        self.compare_mass_information(rigidBodyAPI, 1000.0 * massScale, expectedCoM=Gf.Vec3f(0.0), expectedInertia=Gf.Vec3f(166.667 * massScale))

    # kilogramsPerUnit default change
    def test_mass_rigid_body_cube_decagram_units(self):
        self.setup_scene(kilogramsPerUnit = 0.1)

        # Create test collider cube    
        self.cube = UsdGeom.Cube.Define(self.stage, "/cube")
        self.cube.GetSizeAttr().Set(1.0)
        UsdPhysics.CollisionAPI.Apply(self.cube.GetPrim())
        rigidBodyAPI = UsdPhysics.RigidBodyAPI.Apply(self.cube.GetPrim())

        self.rigidBodyWorldTransform = UsdGeom.Xformable(self.cube.GetPrim()).ComputeLocalToWorldTransform(Usd.TimeCode.Default())
        self.rigidBodyPrim = self.cube.GetPrim()
                        
        massScale = 1.0 / 0.1
        self.compare_mass_information(rigidBodyAPI, 1000.0 * massScale, expectedCoM=Gf.Vec3f(0.0), expectedInertia=Gf.Vec3f(166.667 * massScale))

    # compound rigid body test
    def test_mass_rigid_body_cube_compound(self):
        self.setup_scene()

        # top level xform - rigid body
        self.xform = UsdGeom.Xform.Define(self.stage, "/xform")
        rigidBodyAPI = UsdPhysics.RigidBodyAPI.Apply(self.xform.GetPrim())                

        # Create test collider cube0
        self.cube = UsdGeom.Cube.Define(self.stage, "/xform/cube0")
        self.cube.GetSizeAttr().Set(1.0)
        UsdPhysics.CollisionAPI.Apply(self.cube.GetPrim())
        self.cube.AddTranslateOp().Set(Gf.Vec3f(0, 0, -2.0))

        # Create test colli
        self.cube2 = UsdGeom.Cube.Define(self.stage, "/xform/cube1")
        self.cube2.GetSizeAttr().Set(1.0)
        UsdPhysics.CollisionAPI.Apply(self.cube2.GetPrim())
        self.cube2.AddTranslateOp().Set(Gf.Vec3f(0, 0, 2.0))

        self.rigidBodyWorldTransform = UsdGeom.Xformable(self.xform.GetPrim()).ComputeLocalToWorldTransform(Usd.TimeCode.Default())
        self.rigidBodyPrim = self.xform.GetPrim()
                                
        self.compare_mass_information(rigidBodyAPI, 1000.0 * 2.0, expectedCoM=Gf.Vec3f(0.0))

    def test_mass_rigid_body_nested(self):
        self.setup_scene()

        # Create test nested bodies
        rbo0_xform = UsdGeom.Xform.Define(self.stage, "/rbo0")
        rigidBodyAPI0 = UsdPhysics.RigidBodyAPI.Apply(rbo0_xform.GetPrim())

        cube = UsdGeom.Cube.Define(self.stage, "/rbo0/cube")
        cube.GetSizeAttr().Set(1.0)
        UsdPhysics.CollisionAPI.Apply(cube.GetPrim())

        rbo1_xform = UsdGeom.Xform.Define(self.stage, "/rbo0/rbo1")
        rigidBodyAPI1 = UsdPhysics.RigidBodyAPI.Apply(rbo1_xform.GetPrim())

        cube = UsdGeom.Cube.Define(self.stage, "/rbo0/rbo1/cube")
        cube.GetSizeAttr().Set(2.0)
        UsdPhysics.CollisionAPI.Apply(cube.GetPrim())

        self.rigidBodyWorldTransform = UsdGeom.Xformable(
            rbo0_xform.GetPrim()).ComputeLocalToWorldTransform(
                Usd.TimeCode.Default())
        self.rigidBodyPrim = rbo0_xform.GetPrim()
                        
        self.compare_mass_information(
            rigidBodyAPI0, 1000.0, expectedCoM=Gf.Vec3f(0.0), 
            expectedInertia=Gf.Vec3f(166.667))

        self.rigidBodyWorldTransform = UsdGeom.Xformable(
            rbo1_xform.GetPrim()).ComputeLocalToWorldTransform(
                Usd.TimeCode.Default())
        self.rigidBodyPrim = rbo1_xform.GetPrim()
                        
        self.compare_mass_information(
            rigidBodyAPI1, 8000.0, expectedCoM=Gf.Vec3f(0.0))

    # instance proxy collider test - collider comes from an internal reference
    # on an instanceable prim; the instance proxy must still be found during
    # mass aggregation.
    def test_mass_rigid_body_instance_proxy_collider(self):
        self.setup_scene()

        # Prototype collider authored outside the rigid-body hierarchy.
        self.stage.OverridePrim("/Prototype_Collisions")
        cube = UsdGeom.Cube.Define(
            self.stage, "/Prototype_Collisions/cube")
        cube.CreateSizeAttr(1.0)
        UsdPhysics.CollisionAPI.Apply(cube.GetPrim())

        # Body with density only; mass is computed from colliders.
        bodyXform = UsdGeom.Xform.Define(self.stage, "/World/Body")
        body = bodyXform.GetPrim()
        rigidBodyAPI = UsdPhysics.RigidBodyAPI.Apply(body)
        UsdPhysics.MassAPI.Apply(body).CreateDensityAttr().Set(1000.0)

        # Reference the prototype under the body and make it instanceable.
        collisions = self.stage.DefinePrim("/World/Body/collisions")
        collisions.GetReferences().AddInternalReference(
            "/Prototype_Collisions")
        collisions.SetInstanceable(True)

        self.rigidBodyWorldTransform = \
            UsdGeom.Xformable(body).ComputeLocalToWorldTransform(
                Usd.TimeCode.Default())
        self.rigidBodyPrim = body

        # Expected: same as a unit cube at default density (1000 * 1^3).
        self.compare_mass_information(
            rigidBodyAPI, 1000.0, expectedCoM=Gf.Vec3f(0.0),
            expectedInertia=Gf.Vec3f(166.667))

if __name__ == "__main__":
    unittest.main()
