#!/pxrpythonsubst
#
# Copyright 2021 Pixar
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

if __name__ == "__main__":
    unittest.main()
