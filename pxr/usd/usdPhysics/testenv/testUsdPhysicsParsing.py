#!/pxrpythonsubst
#
# Copyright 2024 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

import os, unittest
from pxr import Usd, UsdPhysics, Gf, UsdGeom, Sdf, UsdShade


toleranceEpsilon = 0.01


class TestUsdPhysicsParsing(unittest.TestCase):

    def test_scene_parse(self):
        stage = Usd.Stage.CreateInMemory()
        self.assertTrue(stage)

        scene = UsdPhysics.Scene.Define(stage, '/physicsScene')
        self.assertTrue(scene)        

        ret_dict = UsdPhysics.LoadUsdPhysicsFromRange(stage, ["/"])

        scene_found = False
        
        for key, value in ret_dict.items():
            if key == UsdPhysics.ObjectType.Scene:                
                scene_found = True
                paths, scene_descs = value
                for path, scene_desc in zip(paths, scene_descs):
                    self.assertTrue(path == scene.GetPrim().GetPrimPath())
                    if UsdGeom.GetStageUpAxis(stage) == UsdGeom.Tokens.y:
                        self.assertTrue(Gf.IsClose(scene_desc.gravityDirection, 
                                                   Gf.Vec3f(0.0, -1.0, 0.0), 
                                                   toleranceEpsilon))
                    elif UsdGeom.GetStageUpAxis(stage) == UsdGeom.Tokens.z:
                        self.assertTrue(Gf.IsClose(scene_desc.gravityDirection, 
                                                   Gf.Vec3f(0.0, 0.0, -1.0), 
                                                   toleranceEpsilon))
                    self.assertAlmostEqual(scene_desc.gravityMagnitude, 
                                           981.0, delta=toleranceEpsilon)
                    self.assertAlmostEqual(scene_desc.gravityMagnitude, 
                                           981.0, delta=toleranceEpsilon)

        self.assertTrue(scene_found)

    def test_collision_parse(self):
        stage = Usd.Stage.CreateInMemory()
        self.assertTrue(stage)

        scene = UsdPhysics.Scene.Define(stage, '/physicsScene')
        self.assertTrue(scene)

        shape_prim = None
        shapes = {
            "sphere" : {"type" : UsdGeom.Sphere, "radius" : 30},
            "capsule" : {"type" : UsdGeom.Capsule, "radius" : 30, "height" : 10, 
                         "axis" : UsdGeom.Tokens.y},
            "capsule_1" : {"type" : UsdGeom.Capsule_1, "topRadius" : 30,
                         "bottomRadius" : 20, "height" : 10, 
                         "axis" : UsdGeom.Tokens.y},
            "cone" : {"type" : UsdGeom.Cone, "radius" : 30, "height" : 10, 
                      "axis" : UsdGeom.Tokens.z},
            "cylinder" : {"type" : UsdGeom.Cylinder, "radius" : 30, 
                          "height" : 10, "axis" : UsdGeom.Tokens.y},
            "cylinder_1" : {"type" : UsdGeom.Cylinder_1, "topRadius" : 30,
                         "bottomRadius" : 20, "height" : 10, 
                         "axis" : UsdGeom.Tokens.y},
            "plane" : {"type" : UsdGeom.Plane, "axis" : UsdGeom.Tokens.z},
            "mesh" : {"type" : UsdGeom.Mesh},
            "points" : {"type" : UsdGeom.Points},
        }

        for key, params in shapes.items():
            if shape_prim is not None:
                stage.RemovePrim(shape_prim.GetPrimPath())
            shape = params["type"].Define(stage, "/Sphere")
            shape_prim = shape.GetPrim()
            
            if (key == "sphere"):
                shape.GetRadiusAttr().Set(params["radius"])
            elif (key == "capsule") or (key == "cone") or (key == "cylinder"):
                shape.GetRadiusAttr().Set(params["radius"])
                shape.GetHeightAttr().Set(params["height"])
                shape.GetAxisAttr().Set(params["axis"])
            elif (key == "capsule_1") or (key == "cylinder_1"):
                shape.GetRadiusTopAttr().Set(params["topRadius"])
                shape.GetRadiusBottomAttr().Set(params["bottomRadius"])
                shape.GetHeightAttr().Set(params["height"])
                shape.GetAxisAttr().Set(params["axis"])
            elif (key == "plane"):
                shape.GetAxisAttr().Set(params["axis"])
            elif (key == "mesh"):
                meshColAPI = UsdPhysics.MeshCollisionAPI.Apply(shape.GetPrim())
                meshColAPI.GetApproximationAttr().Set(
                    UsdPhysics.Tokens.convexHull)
            elif (key == "points"):
                shape.GetWidthsAttr().Set([5.0, 10.0])
                shape.GetPointsAttr().Set([Gf.Vec3f(1.0), Gf.Vec3f(2.0)])

            position = Gf.Vec3f(100.0, 20.0, 10.0)
            rotate_xyz = Gf.Vec3f(0.0, 0.0, 45.0)
            scale = Gf.Vec3f(3.0, 3.0, 3.0)

            shape.AddTranslateOp().Set(position)
            shape.AddRotateXYZOp().Set(rotate_xyz)
            shape.AddScaleOp().Set(scale)

            UsdPhysics.CollisionAPI.Apply(shape_prim)

            ret_dict = UsdPhysics.LoadUsdPhysicsFromRange(stage, ["/"])

            scene_found = False
            num_shape_found = 0

            def compare_shape_params(desc):
                self.assertTrue(prim_path == shape_prim.GetPrimPath())
                self.assertTrue(desc.rigidBody == Sdf.Path())
                self.assertTrue(desc.collisionEnabled is True)
                self.assertTrue(len(desc.materials) == 0)                
                self.assertTrue(len(desc.simulationOwners) == 0)
                self.assertTrue(len(desc.filteredCollisions) == 0)

                # transformation
                self.assertTrue(Gf.IsClose(position, desc.localPos, 
                                           toleranceEpsilon))
                r = Gf.Rotation(desc.localRot)
                eulerAngles = r.Decompose(Gf.Vec3d.XAxis(), Gf.Vec3d.YAxis(), 
                                          Gf.Vec3d.ZAxis())
                self.assertTrue(Gf.IsClose(Gf.Vec3f(eulerAngles), rotate_xyz, 
                                           toleranceEpsilon))
                self.assertTrue(Gf.IsClose(scale, desc.localScale, 
                                           toleranceEpsilon))

            for key, value in ret_dict.items():
                prim_paths, descs = value
                if key == UsdPhysics.ObjectType.Scene:
                    scene_found = True
                elif key == UsdPhysics.ObjectType.SphereShape:
                    for prim_path, desc in zip(prim_paths, descs):
                        # common shape
                        compare_shape_params(desc)

                        # sphere shape
                        self.assertEqual(desc.radius, 
                                         params["radius"] * scale[0])

                        num_shape_found = num_shape_found + 1
                elif key == UsdPhysics.ObjectType.CapsuleShape:
                    for prim_path, desc in zip(prim_paths, descs):
                        # common shape
                        compare_shape_params(desc)

                        # capsule shape
                        self.assertEqual(desc.radius, 
                                         params["radius"] * scale[0])
                        self.assertEqual(desc.halfHeight, 
                                         params["height"] * 0.5 * scale[0])
                        self.assertEqual(desc.axis, UsdPhysics.Axis.Y)

                        num_shape_found = num_shape_found + 1
                elif key == UsdPhysics.ObjectType.Capsule1Shape:
                    for prim_path, desc in zip(prim_paths, descs):
                        # common shape
                        compare_shape_params(desc)

                        # capsule_1 shape
                        self.assertEqual(desc.topRadius, 
                                         params["topRadius"] * scale[0])
                        self.assertEqual(desc.bottomRadius, 
                                         params["bottomRadius"] * scale[0])
                        self.assertEqual(desc.halfHeight, 
                                         params["height"] * 0.5 * scale[0])
                        self.assertEqual(desc.axis, UsdPhysics.Axis.Y)

                        num_shape_found = num_shape_found + 1
                elif key == UsdPhysics.ObjectType.ConeShape:
                    for prim_path, desc in zip(prim_paths, descs):
                        # common shape
                        compare_shape_params(desc)

                        # cone shape
                        self.assertEqual(desc.radius, 
                                params["radius"] * scale[0])
                        self.assertEqual(desc.halfHeight, 
                                         params["height"] * 0.5 * scale[0])
                        self.assertEqual(desc.axis, UsdPhysics.Axis.Z)

                        num_shape_found = num_shape_found + 1
                elif key == UsdPhysics.ObjectType.CylinderShape:
                    for prim_path, desc in zip(prim_paths, descs):
                        # common shape
                        compare_shape_params(desc)

                        # cylinder shape
                        self.assertEqual(desc.radius, 
                                         params["radius"] * scale[0])
                        self.assertEqual(desc.halfHeight, 
                                         params["height"] * 0.5 * scale[0])
                        self.assertEqual(desc.axis, UsdPhysics.Axis.Y)

                        num_shape_found = num_shape_found + 1
                elif key == UsdPhysics.ObjectType.Cylinder1Shape:
                    for prim_path, desc in zip(prim_paths, descs):
                        # common shape
                        compare_shape_params(desc)

                        # capsule_1 shape
                        self.assertEqual(desc.topRadius, 
                                         params["topRadius"] * scale[0])
                        self.assertEqual(desc.bottomRadius, 
                                         params["bottomRadius"] * scale[0])
                        self.assertEqual(desc.halfHeight, 
                                         params["height"] * 0.5 * scale[0])
                        self.assertEqual(desc.axis, UsdPhysics.Axis.Y)

                        num_shape_found = num_shape_found + 1
                elif key == UsdPhysics.ObjectType.PlaneShape:
                    for prim_path, desc in zip(prim_paths, descs):
                        # common shape
                        compare_shape_params(desc)

                        # plane shape
                        self.assertEqual(desc.axis, UsdPhysics.Axis.Z)

                        num_shape_found = num_shape_found + 1
                elif key == UsdPhysics.ObjectType.MeshShape:
                    for prim_path, desc in zip(prim_paths, descs):
                        # common shape
                        compare_shape_params(desc)

                        # mesh shape
                        self.assertTrue(desc.approximation == 
                                        UsdPhysics.Tokens.convexHull)
                        self.assertTrue(desc.doubleSided is not True)
                        self.assertTrue(Gf.IsClose(scale, desc.meshScale, 
                                                   toleranceEpsilon))

                        num_shape_found = num_shape_found + 1
                elif key == UsdPhysics.ObjectType.SpherePointsShape:
                    for prim_path, desc in zip(prim_paths, descs):
                        # common shape
                        compare_shape_params(desc)

                        # sphere points shape
                        self.assertTrue(len(desc.spherePoints) == 2)
                        self.assertTrue(
                            desc.spherePoints[0].center == Gf.Vec3f(1.0))
                        self.assertTrue(
                            desc.spherePoints[1].center == Gf.Vec3f(2.0))
                        # scale * width * 0.5
                        self.assertTrue(
                            desc.spherePoints[0].radius == 7.5)
                        # scale * width * 0.5
                        self.assertTrue(
                            desc.spherePoints[1].radius == 15.0)

                        num_shape_found = num_shape_found + 1

            self.assertTrue(scene_found)
            self.assertTrue(num_shape_found == 1)

    def test_rigidbody_parse(self):
        stage = Usd.Stage.CreateInMemory()
        self.assertTrue(stage)

        scene = UsdPhysics.Scene.Define(stage, '/physicsScene')
        self.assertTrue(scene)            

        rigidbody = UsdGeom.Xform.Define(stage, "/rigidBody")
        rboAPI = UsdPhysics.RigidBodyAPI.Apply(rigidbody.GetPrim())
        position = Gf.Vec3f(100.0, 20.0, 10.0)
        rotate_xyz = Gf.Vec3f(0.0, 0.0, 45.0)
        scale = Gf.Vec3f(3.0, 3.0, 3.0)

        rigidbody.AddTranslateOp().Set(position)
        rigidbody.AddRotateXYZOp().Set(rotate_xyz)
        rigidbody.AddScaleOp().Set(scale)

        velocity = Gf.Vec3f(20.0, 10.0, 5.0)
        angular_vel = Gf.Vec3f(10.0, 1.0, 2.0)
        rboAPI.GetVelocityAttr().Set(velocity)
        rboAPI.GetAngularVelocityAttr().Set(angular_vel)

        ret_dict = UsdPhysics.LoadUsdPhysicsFromRange(stage, ["/"])

        scene_found = False
        rigidbody_found = False
        
        for key, value in ret_dict.items():
            prim_paths, descs = value
            if key == UsdPhysics.ObjectType.Scene:
                for prim_path, desc in zip(prim_paths, descs):
                    scene_found = True
            elif key == UsdPhysics.ObjectType.RigidBody:
                for prim_path, desc in zip(prim_paths, descs):
                    rigidbody_found = True
                    self.assertTrue(prim_path == 
                                    rigidbody.GetPrim().GetPrimPath())
                    self.assertTrue(len(desc.collisions) == 0)
                    self.assertTrue(len(desc.filteredCollisions) == 0)
                    self.assertTrue(len(desc.simulationOwners) == 0)

                    # transformation
                    self.assertTrue(Gf.IsClose(position, desc.position, 
                                               toleranceEpsilon))
                    r = Gf.Rotation(desc.rotation)
                    eulerAngles = r.Decompose(Gf.Vec3d.XAxis(), 
                                              Gf.Vec3d.YAxis(), 
                                              Gf.Vec3d.ZAxis())
                    self.assertTrue(Gf.IsClose(Gf.Vec3f(eulerAngles), 
                                               rotate_xyz, toleranceEpsilon))
                    self.assertTrue(Gf.IsClose(scale, desc.scale, 
                                               toleranceEpsilon))

                    self.assertTrue(desc.rigidBodyEnabled is True)
                    self.assertTrue(desc.kinematicBody is not True)
                    self.assertTrue(desc.startsAsleep is not True)
                    
                    self.assertTrue(Gf.IsClose(velocity, desc.linearVelocity, 
                                               toleranceEpsilon))
                    self.assertTrue(Gf.IsClose(angular_vel, 
                                               desc.angularVelocity, 
                                               toleranceEpsilon))

        self.assertTrue(scene_found)
        self.assertTrue(rigidbody_found)

    def test_rigidbody_collision_parse(self):
        stage = Usd.Stage.CreateInMemory()
        self.assertTrue(stage)

        scene = UsdPhysics.Scene.Define(stage, '/physicsScene')
        self.assertTrue(scene)            

        rigidbody = UsdGeom.Xform.Define(stage, "/rigidBody")
        UsdPhysics.RigidBodyAPI.Apply(rigidbody.GetPrim())
        rb_position = Gf.Vec3f(100.0, 0.0, 0.0)
        rb_rotate_xyz = Gf.Vec3f(0.0, 0.0, 0.0)
        rb_scale = Gf.Vec3f(3.0, 3.0, 3.0)

        rigidbody.AddTranslateOp().Set(rb_position)
        rigidbody.AddRotateXYZOp().Set(rb_rotate_xyz)
        rigidbody.AddScaleOp().Set(rb_scale)

        cube = UsdGeom.Cube.Define(stage, "/rigidBody/cube")
        UsdPhysics.CollisionAPI.Apply(cube.GetPrim())        

        ret_dict = UsdPhysics.LoadUsdPhysicsFromRange(stage, ["/"])

        scene_found = False
        rigidbody_found = False
        cube_found = False
        
        for key, value in ret_dict.items():
            prim_paths, descs = value
            if key == UsdPhysics.ObjectType.Scene:
                for prim_path, desc in zip(prim_paths, descs):
                    scene_found = True
            elif key == UsdPhysics.ObjectType.RigidBody:
                for prim_path, desc in zip(prim_paths, descs):
                    rigidbody_found = True
                    self.assertTrue(prim_path == 
                                    rigidbody.GetPrim().GetPrimPath())
                    self.assertTrue(len(desc.collisions) == 1)
                    self.assertTrue(desc.collisions[0] == 
                                    cube.GetPrim().GetPrimPath())

                    # transformation
                    self.assertTrue(Gf.IsClose(rb_position, desc.position, 
                                               toleranceEpsilon))
                    r = Gf.Rotation(desc.rotation)
                    eulerAngles = r.Decompose(Gf.Vec3d.XAxis(), 
                                              Gf.Vec3d.YAxis(), 
                                              Gf.Vec3d.ZAxis())
                    self.assertTrue(Gf.IsClose(Gf.Vec3f(eulerAngles), 
                                               rb_rotate_xyz, toleranceEpsilon))
                    self.assertTrue(Gf.IsClose(rb_scale, desc.scale, 
                                               toleranceEpsilon))
            elif key == UsdPhysics.ObjectType.CubeShape:
                for prim_path, desc in zip(prim_paths, descs):
                    cube_found = True
                    self.assertTrue(prim_path == cube.GetPrim().GetPrimPath())
                    self.assertTrue(desc.rigidBody == 
                                    rigidbody.GetPrim().GetPrimPath())

                    # transformation
                    self.assertTrue(Gf.IsClose(Gf.Vec3f(0.0), desc.localPos, 
                                               toleranceEpsilon))
                    r = Gf.Rotation(desc.localRot)
                    eulerAngles = r.Decompose(Gf.Vec3d.XAxis(), 
                                              Gf.Vec3d.YAxis(), 
                                              Gf.Vec3d.ZAxis())
                    self.assertTrue(Gf.IsClose(Gf.Vec3f(eulerAngles), 
                                               Gf.Vec3f(0.0), toleranceEpsilon))
                    self.assertTrue(Gf.IsClose(Gf.Vec3f(1.0), desc.localScale, 
                                               toleranceEpsilon))

        self.assertTrue(scene_found)
        self.assertTrue(rigidbody_found)
        self.assertTrue(cube_found)

    def test_filtering_pairs_parse(self):
        stage = Usd.Stage.CreateInMemory()
        self.assertTrue(stage)

        scene = UsdPhysics.Scene.Define(stage, '/physicsScene')
        self.assertTrue(scene)            

        rigidbody_0 = UsdGeom.Xform.Define(stage, "/rigidBody0")
        UsdPhysics.RigidBodyAPI.Apply(rigidbody_0.GetPrim())

        cube_0 = UsdGeom.Cube.Define(stage, "/rigidBody0/cube")
        UsdPhysics.CollisionAPI.Apply(cube_0.GetPrim())

        rigidbody_1 = UsdGeom.Xform.Define(stage, "/rigidBody1")
        UsdPhysics.RigidBodyAPI.Apply(rigidbody_1.GetPrim())
        rb_filter = UsdPhysics.FilteredPairsAPI.Apply(rigidbody_1.GetPrim())
        rb_filter.GetFilteredPairsRel().AddTarget(
            rigidbody_0.GetPrim().GetPrimPath())

        cube_1 = UsdGeom.Cube.Define(stage, "/rigidBody1/cube")
        UsdPhysics.CollisionAPI.Apply(cube_1.GetPrim())
        col_filter = UsdPhysics.FilteredPairsAPI.Apply(cube_1.GetPrim())
        col_filter.GetFilteredPairsRel().AddTarget(
            cube_0.GetPrim().GetPrimPath())

        ret_dict = UsdPhysics.LoadUsdPhysicsFromRange(stage, ["/"])

        scene_found = False
        rigidbody_count = 0
        cube_count = 0
        
        for key, value in ret_dict.items():
            prim_paths, descs = value
            if key == UsdPhysics.ObjectType.Scene:
                for prim_path, desc in zip(prim_paths, descs):
                    scene_found = True
            elif key == UsdPhysics.ObjectType.RigidBody:
                for prim_path, desc in zip(prim_paths, descs):
                    rigidbody_count = rigidbody_count + 1
                    if prim_path == rigidbody_1.GetPrim().GetPrimPath():
                        self.assertTrue(len(desc.filteredCollisions) == 1)
                        self.assertTrue(desc.filteredCollisions[0] == 
                                        rigidbody_0.GetPrim().GetPrimPath())
            elif key == UsdPhysics.ObjectType.CubeShape:
                for prim_path, desc in zip(prim_paths, descs):
                    cube_count = cube_count + 1
                    if prim_path == cube_1.GetPrim().GetPrimPath():
                        self.assertTrue(len(desc.filteredCollisions) == 1)
                        self.assertTrue(desc.filteredCollisions[0] == 
                                        cube_0.GetPrim().GetPrimPath())

        self.assertTrue(scene_found)
        self.assertTrue(rigidbody_count == 2)
        self.assertTrue(cube_count == 2)

    # materials
    def test_collision_material_parse(self):
        stage = Usd.Stage.CreateInMemory()
        self.assertTrue(stage)

        scene = UsdPhysics.Scene.Define(stage, '/physicsScene')
        self.assertTrue(scene)            

        cube = UsdGeom.Cube.Define(stage, "/cube")
        UsdPhysics.CollisionAPI.Apply(cube.GetPrim())

        # single material
        materialPrim = UsdShade.Material.Define(stage, "/physicsMaterial")
        UsdPhysics.MaterialAPI.Apply(materialPrim.GetPrim())
        bindingAPI = UsdShade.MaterialBindingAPI.Apply(cube.GetPrim())
        bindingAPI.Bind(materialPrim, UsdShade.Tokens.weakerThanDescendants, 
                        "physics")

        ret_dict = UsdPhysics.LoadUsdPhysicsFromRange(stage, ["/"])

        scene_found = False
        cube_found = False
        material_found = False
        
        for key, value in ret_dict.items():
            prim_paths, descs = value
            if key == UsdPhysics.ObjectType.Scene:
                for prim_path, desc in zip(prim_paths, descs):
                    scene_found = True
            elif key == UsdPhysics.ObjectType.CubeShape:
                for prim_path, desc in zip(prim_paths, descs):
                    cube_found = True
                    self.assertTrue(len(desc.materials) == 1)
                    desc.materials[0] = materialPrim.GetPrim().GetPrimPath()
            elif key == UsdPhysics.ObjectType.RigidBodyMaterial:
                for prim_path, desc in zip(prim_paths, descs):
                    material_found = True
                    self.assertTrue(prim_path == 
                                    materialPrim.GetPrim().GetPrimPath())
                    self.assertEqual(desc.staticFriction, 0)
                    self.assertEqual(desc.dynamicFriction, 0)
                    self.assertEqual(desc.restitution, 0)
                    self.assertEqual(desc.density, 0)

        self.assertTrue(scene_found)
        self.assertTrue(cube_found)
        self.assertTrue(material_found)

    def test_collision_multi_material_parse(self):
        stage = Usd.Stage.CreateInMemory()
        self.assertTrue(stage)

        scene = UsdPhysics.Scene.Define(stage, '/physicsScene')
        self.assertTrue(scene)            

        mesh = UsdGeom.Mesh.Define(stage, "/mesh")
        UsdPhysics.CollisionAPI.Apply(mesh.GetPrim())
        meshColAPI = UsdPhysics.MeshCollisionAPI.Apply(mesh.GetPrim())
        meshColAPI.GetApproximationAttr().Set(UsdPhysics.Tokens.none)

        # materials        
        materialPrim0 = UsdShade.Material.Define(stage, "/physicsMaterial0")
        UsdPhysics.MaterialAPI.Apply(materialPrim0.GetPrim())
        materialPrim1 = UsdShade.Material.Define(stage, "/physicsMaterial1")
        UsdPhysics.MaterialAPI.Apply(materialPrim1.GetPrim())

        materials = [materialPrim0, materialPrim1]

        # Fill in VtArrays
        points = []
        normals = []
        indices = []
        vertexCounts = []

        stripSize = 100.0
        halfSize = 500.0 

        for i in range(2):
            subset = UsdGeom.Subset.Define(stage, "/mesh/subset" + str(i))
            subset.CreateElementTypeAttr().Set("face")
            subset_indices = [i]

            bindingAPI = UsdShade.MaterialBindingAPI.Apply(subset.GetPrim())
            bindingAPI.Bind(materials[i] , 
                            UsdShade.Tokens.weakerThanDescendants, 
                            "physics")
            # rel = subset.GetPrim().CreateRelationship(
            #    "material:binding:physics", False)
            # rel.SetTargets([materials[i].GetPrim().GetPrimPath()])

            points.append(Gf.Vec3f(-stripSize / 2.0 + stripSize * i, 
                                   -halfSize, 0.0))
            points.append(Gf.Vec3f(-stripSize / 2.0 + stripSize * (i + 1), 
                                   -halfSize, 0.0))
            points.append(Gf.Vec3f(-stripSize / 2.0 + stripSize * (i + 1), 
                                   halfSize, 0.0))
            points.append(Gf.Vec3f(-stripSize / 2.0 + stripSize * i, 
                                   halfSize, 0.0))
            
            for j in range(4):
                normals.append(Gf.Vec3f(0, 0, 1))
                indices.append(j + i * 4)                

            subset.CreateIndicesAttr().Set(subset_indices)
            vertexCounts.append(4)

        mesh.CreateFaceVertexCountsAttr().Set(vertexCounts)
        mesh.CreateFaceVertexIndicesAttr().Set(indices)
        mesh.CreatePointsAttr().Set(points)
        mesh.CreateDoubleSidedAttr().Set(False)
        mesh.CreateNormalsAttr().Set(normals)

        ret_dict = UsdPhysics.LoadUsdPhysicsFromRange(stage, ["/"])

        scene_found = False
        mesh_found = False
        num_materials = 0
        
        for key, value in ret_dict.items():
            prim_paths, descs = value
            if key == UsdPhysics.ObjectType.Scene:
                for prim_path, desc in zip(prim_paths, descs):
                    scene_found = True
            elif key == UsdPhysics.ObjectType.MeshShape:
                for prim_path, desc in zip(prim_paths, descs):
                    mesh_found = True
                    self.assertTrue(len(desc.materials) == 2)
                    self.assertTrue(desc.materials[0] == 
                                    materialPrim0.GetPrim().GetPrimPath())
                    self.assertTrue(desc.materials[1] == 
                                    materialPrim1.GetPrim().GetPrimPath())                    
            elif key == UsdPhysics.ObjectType.RigidBodyMaterial:
                for prim_path, desc in zip(prim_paths, descs):
                    num_materials = num_materials + 1

        self.assertTrue(scene_found)
        self.assertTrue(mesh_found)
        self.assertTrue(num_materials == 2)

    # joints
    def test_joint_parse(self):
        stage = Usd.Stage.CreateInMemory()
        self.assertTrue(stage)

        scene = UsdPhysics.Scene.Define(stage, '/physicsScene')
        self.assertTrue(scene)

        rigid_body_0 = UsdGeom.Xform.Define(stage, "/rigidBody0")
        UsdPhysics.RigidBodyAPI.Apply(rigid_body_0.GetPrim())
        rigid_body_1 = UsdGeom.Xform.Define(stage, "/rigidBody1")
        UsdPhysics.RigidBodyAPI.Apply(rigid_body_1.GetPrim())

        joint_prim = None
        joints = {
            "fixed" : {"type" : UsdPhysics.FixedJoint},
            "revolute0" : {"type" : UsdPhysics.RevoluteJoint, 
                           "axis" : UsdGeom.Tokens.y, "limit" : True, 
                           "drive" : True},
            "revolute1" : {"type" : UsdPhysics.RevoluteJoint, 
                           "axis" : UsdGeom.Tokens.y, "limit" : False, 
                           "drive" : False},
            "prismatic0" : {"type" : UsdPhysics.PrismaticJoint, 
                            "axis" : UsdGeom.Tokens.z, "limit" : True, 
                            "drive" : True},
            "prismatic1" : {"type" : UsdPhysics.PrismaticJoint, 
                            "axis" : UsdGeom.Tokens.z, "limit" : False, 
                            "drive" : False},
            "spherical" : {"type" : UsdPhysics.SphericalJoint, 
                           "axis" : UsdGeom.Tokens.z},
            "distance" : {"type" : UsdPhysics.DistanceJoint},
            "d6" : {"type" : UsdPhysics.Joint},
        }

        for key, params in joints.items():
            if joint_prim is not None:
                stage.RemovePrim(joint_prim.GetPrimPath())
            joint = params["type"].Define(stage, "/joint")
            joint_prim = joint.GetPrim()

            # common part
            joint.GetBody0Rel().AddTarget(rigid_body_0.GetPrim().GetPrimPath())
            joint.GetBody1Rel().AddTarget(rigid_body_1.GetPrim().GetPrimPath())

            joint.GetBreakForceAttr().Set(500)
            joint.GetBreakTorqueAttr().Set(1500)

            joint.GetLocalPos0Attr().Set(Gf.Vec3f(1.0))
            joint.GetLocalPos1Attr().Set(Gf.Vec3f(-1.0))

            if (key == "revolute0") or (key == "revolute1"):
                joint.GetAxisAttr().Set(params["axis"])
                if (params["limit"]):
                    joint.GetLowerLimitAttr().Set(0)
                    joint.GetUpperLimitAttr().Set(90)
                if (params["drive"]):
                    drive = UsdPhysics.DriveAPI.Apply(joint.GetPrim(), 
                                                      UsdPhysics.Tokens.angular)
                    drive.GetTargetPositionAttr().Set(10)
                    drive.GetTargetVelocityAttr().Set(20)
                    drive.GetStiffnessAttr().Set(30)
                    drive.GetDampingAttr().Set(40)
            elif (key == "prismatic0") or (key == "prismatic1"):
                joint.GetAxisAttr().Set(params["axis"])
                if (params["limit"]):
                    joint.GetLowerLimitAttr().Set(10)
                    joint.GetUpperLimitAttr().Set(80)
                if (params["drive"]):
                    drive = UsdPhysics.DriveAPI.Apply(joint.GetPrim(), 
                                                      UsdPhysics.Tokens.linear)
                    drive.GetTargetPositionAttr().Set(10)
                    drive.GetTargetVelocityAttr().Set(20)
                    drive.GetStiffnessAttr().Set(30)
                    drive.GetDampingAttr().Set(40)
            elif (key == "spherical"):
                joint.GetAxisAttr().Set(params["axis"])
                joint.CreateConeAngle0LimitAttr().Set(20)
                joint.CreateConeAngle1LimitAttr().Set(30)
            elif (key == "distance"):
                joint.GetMinDistanceAttr().Set(0)
                joint.GetMaxDistanceAttr().Set(10)
            elif (key == "d6"):
                x_trans_lim = UsdPhysics.LimitAPI.Apply(joint.GetPrim(), 
                                                        UsdPhysics.Tokens.transX)
                x_trans_lim.CreateLowAttr().Set(-10)
                x_trans_lim.CreateHighAttr().Set(10)
                y_trans_lim = UsdPhysics.LimitAPI.Apply(joint.GetPrim(), 
                                                        UsdPhysics.Tokens.transY)
                y_trans_lim.CreateLowAttr().Set(-20)
                y_trans_lim.CreateHighAttr().Set(20)

                x_rot_lim = UsdPhysics.LimitAPI.Apply(joint.GetPrim(), 
                                                      UsdPhysics.Tokens.rotX)
                x_rot_lim.CreateLowAttr().Set(-30)
                x_rot_lim.CreateHighAttr().Set(30)
                y_rot_lim = UsdPhysics.LimitAPI.Apply(joint.GetPrim(), 
                                                      UsdPhysics.Tokens.rotY)
                y_rot_lim.CreateLowAttr().Set(30)
                y_rot_lim.CreateHighAttr().Set(-30)

                drive = UsdPhysics.DriveAPI.Apply(joint.GetPrim(), 
                                                  UsdPhysics.Tokens.rotX)
                drive.GetTargetPositionAttr().Set(10)
                drive.GetTargetVelocityAttr().Set(20)
                drive.GetStiffnessAttr().Set(30)
                drive.GetDampingAttr().Set(40)

            ret_dict = UsdPhysics.LoadUsdPhysicsFromRange(stage, ["/"])

            scene_found = False
            num_joints_found = 0

            def compare_joint_params(desc):
                self.assertTrue(prim_path == joint_prim.GetPrimPath())

                self.assertTrue(desc.rel0 == 
                                rigid_body_0.GetPrim().GetPrimPath())
                self.assertTrue(desc.body0 == 
                                rigid_body_0.GetPrim().GetPrimPath())
                self.assertTrue(desc.rel1 == 
                                rigid_body_1.GetPrim().GetPrimPath())
                self.assertTrue(desc.body1 == 
                                rigid_body_1.GetPrim().GetPrimPath())

                self.assertTrue(Gf.IsClose(desc.localPose0Position, 
                                           Gf.Vec3f(1.0), toleranceEpsilon))
                self.assertTrue(Gf.IsClose(desc.localPose1Position, 
                                           Gf.Vec3f(-1.0), toleranceEpsilon))

                self.assertTrue(desc.jointEnabled is True)
                self.assertTrue(desc.collisionEnabled is False)
                self.assertTrue(desc.excludeFromArticulation is False)

                self.assertEqual(desc.breakForce, 500)
                self.assertEqual(desc.breakTorque, 1500)

            for key, value in ret_dict.items():
                prim_paths, descs = value
                if key == UsdPhysics.ObjectType.Scene:
                    scene_found = True
                elif key == UsdPhysics.ObjectType.FixedJoint:
                    for prim_path, desc in zip(prim_paths, descs):
                        # common joint
                        compare_joint_params(desc)

                        num_joints_found = num_joints_found + 1
                elif key == UsdPhysics.ObjectType.RevoluteJoint:
                    for prim_path, desc in zip(prim_paths, descs):
                        # common joint
                        compare_joint_params(desc)

                        # revolute joint part
                        self.assertTrue(desc.axis == UsdPhysics.Axis.Y)

                        # limit
                        if (params["limit"]):
                            self.assertTrue(desc.limit.enabled is True)
                            self.assertEqual(desc.limit.lower, 0)
                            self.assertEqual(desc.limit.upper, 90)
                        else:
                            self.assertTrue(desc.limit.enabled is False)

                        # drive
                        if (params["drive"]):
                            self.assertTrue(desc.drive.enabled is True)
                            self.assertEqual(desc.drive.targetPosition, 10)
                            self.assertEqual(desc.drive.targetVelocity, 20)
                            self.assertEqual(desc.drive.stiffness, 30)
                            self.assertEqual(desc.drive.damping, 40)
                        else:
                            self.assertTrue(desc.drive.enabled is False)

                        num_joints_found = num_joints_found + 1
                elif key == UsdPhysics.ObjectType.PrismaticJoint:
                    for prim_path, desc in zip(prim_paths, descs):
                        # common joint
                        compare_joint_params(desc)

                        # prismatic joint part
                        self.assertTrue(desc.axis == UsdPhysics.Axis.Z)

                        # limit
                        if (params["limit"]):                        
                            self.assertTrue(desc.limit.enabled is True)
                            self.assertEqual(desc.limit.lower, 10)
                            self.assertEqual(desc.limit.upper, 80)
                        else:
                            self.assertTrue(desc.limit.enabled is False)

                        # drive
                        if (params["drive"]):
                            self.assertTrue(desc.drive.enabled is True)
                            self.assertEqual(desc.drive.targetPosition, 10)
                            self.assertEqual(desc.drive.targetVelocity, 20)
                            self.assertEqual(desc.drive.stiffness, 30)
                            self.assertEqual(desc.drive.damping, 40)
                        else:
                            self.assertTrue(desc.drive.enabled is False)

                        num_joints_found = num_joints_found + 1
                elif key == UsdPhysics.ObjectType.D6Joint:
                    for prim_path, desc in zip(prim_paths, descs):
                        # common joint
                        compare_joint_params(desc)

                        # d6 joint part
                        self.assertTrue(len(desc.jointLimits) == 4)
                        self.assertTrue(len(desc.jointDrives) == 1)

                        # drive for rotX
                        xDrivePair = desc.jointDrives[0]
                        self.assertTrue(xDrivePair.first == 
                                        UsdPhysics.JointDOF.RotX)
                        xDrive = xDrivePair.second
                        self.assertTrue(xDrive.enabled is True)
                        self.assertEqual(xDrive.targetPosition, 10)
                        self.assertEqual(xDrive.targetVelocity, 20)
                        self.assertEqual(xDrive.stiffness, 30)
                        self.assertEqual(xDrive.damping, 40)

                        # limits
                        for d6_limit in desc.jointLimits:
                            limit_dof = d6_limit.first
                            limit = d6_limit.second
                            if limit_dof == UsdPhysics.JointDOF.TransX:
                                self.assertTrue(limit.enabled is True)
                                self.assertEqual(limit.lower, -10)
                                self.assertEqual(limit.upper, 10)
                            elif limit_dof == UsdPhysics.JointDOF.TransY:
                                self.assertTrue(limit.enabled is True)
                                self.assertEqual(limit.lower, -20)
                                self.assertEqual(limit.upper, 20)
                            elif limit_dof == UsdPhysics.JointDOF.RotX:
                                self.assertTrue(limit.enabled is True)
                                self.assertEqual(limit.lower, -30)
                                self.assertEqual(limit.upper, 30)
                            elif limit_dof == UsdPhysics.JointDOF.RotY:
                                self.assertTrue(limit.enabled is True)
                                # lower higher then upper means DOF is locked
                                self.assertEqual(limit.lower, 30)
                                self.assertEqual(limit.upper, -30)

                        num_joints_found = num_joints_found + 1
                elif key == UsdPhysics.ObjectType.SphericalJoint:
                    for prim_path, desc in zip(prim_paths, descs):
                        # common joint
                        compare_joint_params(desc)

                        # spherical joint part
                        self.assertTrue(desc.axis == UsdPhysics.Axis.Z)
                        self.assertTrue(desc.limit.enabled is True)
                        # lower maps to cone0 angle
                        self.assertEqual(desc.limit.lower, 20)
                        # upper maps to cone1 angle
                        self.assertEqual(desc.limit.upper, 30)

                        num_joints_found = num_joints_found + 1
                elif key == UsdPhysics.ObjectType.DistanceJoint:
                    for prim_path, desc in zip(prim_paths, descs):
                        # common joint
                        compare_joint_params(desc)

                        # distance joint part
                        # limit can be per min/max, limit.enabled not used
                        self.assertTrue(desc.minEnabled is True)
                        self.assertTrue(desc.maxEnabled is True)
                        # lower maps to min
                        self.assertEqual(desc.limit.lower, 0)
                        # upper maps to max
                        self.assertEqual(desc.limit.upper, 10)

                        num_joints_found = num_joints_found + 1

            self.assertTrue(scene_found)
            self.assertTrue(num_joints_found == 1)

    # articulations
    def test_articulation_parse(self):
        stage = Usd.Stage.CreateInMemory()
        self.assertTrue(stage)

        scene = UsdPhysics.Scene.Define(stage, '/physicsScene')
        self.assertTrue(scene)

        # top level xform
        top_xform = UsdGeom.Xform.Define(stage, "/xform")

        rigid_body_0 = UsdGeom.Xform.Define(stage, "/xform/rigidBody0")
        UsdPhysics.RigidBodyAPI.Apply(rigid_body_0.GetPrim())
        rigid_body_1 = UsdGeom.Xform.Define(stage, "/xform/rigidBody1")
        UsdPhysics.RigidBodyAPI.Apply(rigid_body_1.GetPrim())
        rigid_body_2 = UsdGeom.Xform.Define(stage, "/xform/rigidBody2")
        UsdPhysics.RigidBodyAPI.Apply(rigid_body_2.GetPrim())

        joint_0 = UsdPhysics.RevoluteJoint.Define(stage, "/xform/revoluteJoint0")
        joint_0.GetBody0Rel().AddTarget(rigid_body_0.GetPrim().GetPrimPath())
        joint_0.GetBody1Rel().AddTarget(rigid_body_1.GetPrim().GetPrimPath())

        joint_1 = UsdPhysics.RevoluteJoint.Define(stage, "/xform/revoluteJoint1")
        joint_1.GetBody0Rel().AddTarget(rigid_body_1.GetPrim().GetPrimPath())
        joint_1.GetBody1Rel().AddTarget(rigid_body_2.GetPrim().GetPrimPath())

        articulation_api_prim = None
        fixed_joint = None
        articulations = ["floating", "auto", "fixed", "auto_fixed"]

        for type in articulations:
            if articulation_api_prim is not None:
                articulation_api_prim.RemoveAPI(UsdPhysics.ArticulationRootAPI)
                articulation_api_prim = None

            if type == "fixed":
                fixed_joint = UsdPhysics.FixedJoint.Define(stage, 
                                                           "/xform/fixedJoint")
                fixed_joint.GetBody1Rel().AddTarget(
                    rigid_body_0.GetPrim().GetPrimPath())
                UsdPhysics.ArticulationRootAPI.Apply(fixed_joint.GetPrim())
                articulation_api_prim = fixed_joint.GetPrim()
            elif type == "floating":
                UsdPhysics.ArticulationRootAPI.Apply(rigid_body_1.GetPrim())
                articulation_api_prim = rigid_body_1.GetPrim()
            elif type == "auto" or type == "auto_fixed":
                UsdPhysics.ArticulationRootAPI.Apply(top_xform.GetPrim())
                articulation_api_prim = top_xform.GetPrim()

            ret_dict = UsdPhysics.LoadUsdPhysicsFromRange(stage, ["/"])

            scene_found = False
            articulation_found = False

            for key, value in ret_dict.items():
                prim_paths, descs = value
                if key == UsdPhysics.ObjectType.Scene:
                    scene_found = True
                elif key == UsdPhysics.ObjectType.Articulation:
                    for prim_path, desc in zip(prim_paths, descs):
                        articulation_found = True

                        self.assertTrue(len(desc.rootPrims) == 1)

                        if type == "floating" or type == "auto":
                            self.assertTrue(len(desc.articulatedJoints) == 2)
                            self.assertTrue(len(desc.articulatedBodies) == 3)
                            
                            self.assertTrue(desc.rootPrims[0] == 
                                            rigid_body_1.GetPrim().GetPrimPath())
                        else:
                            self.assertTrue(len(desc.articulatedJoints) == 3)
                            # SdfPath returned for the static body
                            self.assertTrue(len(desc.articulatedBodies) == 4)

                            self.assertTrue(desc.rootPrims[0] == 
                                            fixed_joint.GetPrim().GetPrimPath())

            self.assertTrue(scene_found)
            self.assertTrue(articulation_found)

    def test_collision_groups_parse(self):
        stage = Usd.Stage.CreateInMemory()
        self.assertTrue(stage)

        scene = UsdPhysics.Scene.Define(stage, '/physicsScene')
        self.assertTrue(scene)

        num_collision_groups = 10

        collision_groups = []
        for i in range(num_collision_groups):
            collision_group = UsdPhysics.CollisionGroup.Define(
                stage, "/collision_group" + str(i))
            collision_group.GetInvertFilteredGroupsAttr().Set(False)
            collision_groups.append(collision_group)
            if i == 0:
                collision_group.GetFilteredGroupsRel().AddTarget(
                    "/collision_group" + str(num_collision_groups - 1))
            else:
                collision_group.GetFilteredGroupsRel().AddTarget(
                    "/collision_group" + str(i - 1))
                if i % 3 == 0:
                    collision_group.GetMergeGroupNameAttr().Set("three")
                if i % 4 == 0:
                    collision_group.GetMergeGroupNameAttr().Set("four")

        ret_dict = UsdPhysics.LoadUsdPhysicsFromRange(stage, ["/"])

        scene_found = False
        num_reported_collision_groups = 0

        for key, value in ret_dict.items():
            prim_paths, descs = value
            if key == UsdPhysics.ObjectType.Scene:
                scene_found = True
            elif key == UsdPhysics.ObjectType.CollisionGroup:
                for prim_path, desc in zip(prim_paths, descs):
                    if desc.mergeGroupName == "three":
                        self.assertTrue(
                            prim_path == 
                            collision_groups[3].GetPrim().GetPrimPath())
                        self.assertTrue(len(desc.mergedGroups) == 3)
                        self.assertTrue(len(desc.filteredGroups) == 3)                        
                        self.assertTrue(desc.filteredGroups[0] == 
                                        "/collision_group2")
                        self.assertTrue(desc.filteredGroups[1] == 
                                        "/collision_group5")
                        self.assertTrue(desc.filteredGroups[2] == 
                                        "/collision_group8")
                    elif desc.mergeGroupName == "four":
                        self.assertTrue(
                            prim_path == 
                            collision_groups[4].GetPrim().GetPrimPath())
                        self.assertTrue(len(desc.mergedGroups) == 2)
                        self.assertTrue(len(desc.filteredGroups) == 2)                        
                        self.assertTrue(desc.filteredGroups[0] == 
                                        "/collision_group3")
                        self.assertTrue(desc.filteredGroups[1] == 
                                        "/collision_group7")
                    else:
                        self.assertTrue(not desc.mergeGroupName)
                        self.assertTrue(len(desc.mergedGroups) == 0)
                        self.assertTrue(len(desc.filteredGroups) == 1)

                    self.assertTrue(desc.invertFilteredGroups is False)
                    num_reported_collision_groups = \
                        num_reported_collision_groups + 1

        self.assertTrue(scene_found)
        self.assertTrue(num_reported_collision_groups == 7)

    def test_collision_groups_collider_parse(self):
        stage = Usd.Stage.CreateInMemory()
        self.assertTrue(stage)

        scene = UsdPhysics.Scene.Define(stage, '/physicsScene')
        self.assertTrue(scene)

        collision_group_0 = UsdPhysics.CollisionGroup.Define(
            stage, "/collision_group_0")
        collision_group_1 = UsdPhysics.CollisionGroup.Define(
            stage, "/collision_group_1")

        cube = UsdGeom.Cube.Define(stage, "/cube")
        UsdPhysics.CollisionAPI.Apply(cube.GetPrim())

        collision_group_0.GetCollidersCollectionAPI().GetIncludesRel().AddTarget(
            cube.GetPrim().GetPrimPath())
        collision_group_1.GetCollidersCollectionAPI().GetIncludesRel().AddTarget(
            cube.GetPrim().GetPrimPath())

        ret_dict = UsdPhysics.LoadUsdPhysicsFromRange(stage, ["/"])

        scene_found = False
        num_reported_collision_groups = 0
        shape_reported = False

        for key, value in ret_dict.items():
            prim_paths, descs = value
            if key == UsdPhysics.ObjectType.Scene:
                scene_found = True
            elif key == UsdPhysics.ObjectType.CollisionGroup:
                for prim_path, desc in zip(prim_paths, descs):
                    num_reported_collision_groups = \
                        num_reported_collision_groups + 1
            elif key == UsdPhysics.ObjectType.CubeShape:
                for prim_path, desc in zip(prim_paths, descs):
                    self.assertTrue(len(desc.collisionGroups) == 2)
                    self.assertTrue(desc.collisionGroups[0] == 
                                    collision_group_0.GetPrim().GetPrimPath())
                    self.assertTrue(desc.collisionGroups[1] == 
                                    collision_group_1.GetPrim().GetPrimPath())
                    shape_reported = True

        self.assertTrue(scene_found)
        self.assertTrue(num_reported_collision_groups == 2)
        self.assertTrue(shape_reported)

    # custom tokens
    def test_custom_geometry_parse(self):
        stage = Usd.Stage.CreateInMemory()
        self.assertTrue(stage)

        scene = UsdPhysics.Scene.Define(stage, '/physicsScene')
        self.assertTrue(scene)

        UsdGeom.Cube.Define(stage, "/cube")

        layer = stage.GetEditTarget().GetLayer()
        primSpec = Sdf.CreatePrimInLayer(layer, "/cube")
        listOp = Sdf.TokenListOp()
        listOp.prependedItems = ["MyCustomGeometryAPI", "PhysicsCollisionAPI"]
        primSpec.SetInfo(Usd.Tokens.apiSchemas, listOp)

        custom_tokens = UsdPhysics.CustomUsdPhysicsTokens()
        custom_tokens.shapeTokens.append("MyCustomGeometryAPI")

        ret_dict = UsdPhysics.LoadUsdPhysicsFromRange(stage, ["/"], [], 
                custom_tokens)

        scene_found = False
        custom_geometry_found = False

        for key, value in ret_dict.items():
            prim_paths, descs = value
            if key == UsdPhysics.ObjectType.Scene:
                scene_found = True
            elif key == UsdPhysics.ObjectType.CustomShape:
                for prim_path, desc in zip(prim_paths, descs):
                    custom_geometry_found = True

        self.assertTrue(scene_found)
        self.assertTrue(custom_geometry_found)

    # simulation owner tests
    def test_rigid_body_simulation_owner_parse(self):
        stage = Usd.Stage.CreateInMemory()
        self.assertTrue(stage)

        scene = UsdPhysics.Scene.Define(stage, '/physicsScene')
        self.assertTrue(scene)

        cube = UsdGeom.Cube.Define(stage, "/cube")
        rbo_api = UsdPhysics.RigidBodyAPI.Apply(cube.GetPrim())

        custom_tokens = UsdPhysics.CustomUsdPhysicsTokens()
        simulation_owners = [scene.GetPrim().GetPrimPath()]
        ret_dict = UsdPhysics.LoadUsdPhysicsFromRange(stage, ["/"], [], 
                custom_tokens, simulation_owners)

        def check_ret_dict():
            scene_found = False
            rigid_body_found = False

            for key, value in ret_dict.items():
                prim_paths, descs = value
                if key == UsdPhysics.ObjectType.Scene:
                    scene_found = True
                elif key == UsdPhysics.ObjectType.RigidBody:
                    for prim_path, desc in zip(prim_paths, descs):
                        rigid_body_found = True

            return scene_found, rigid_body_found

        scene_found, rigid_body_found = check_ret_dict()

        self.assertTrue(scene_found)
        self.assertTrue(not rigid_body_found)

        simulation_owners = [Sdf.Path()]
        ret_dict = UsdPhysics.LoadUsdPhysicsFromRange(stage, ["/"], [], 
                custom_tokens, simulation_owners)
        scene_found, rigid_body_found = check_ret_dict()

        self.assertTrue(not scene_found)
        self.assertTrue(rigid_body_found)

        simulation_owners = [scene.GetPrim().GetPrimPath()]
        rbo_api.GetSimulationOwnerRel().AddTarget(scene.GetPrim().GetPrimPath())
        ret_dict = UsdPhysics.LoadUsdPhysicsFromRange(stage, ["/"], [], 
                custom_tokens, simulation_owners)
        scene_found, rigid_body_found = check_ret_dict()

        self.assertTrue(scene_found)
        self.assertTrue(rigid_body_found)

    def test_collision_simulation_owner_parse(self):
        stage = Usd.Stage.CreateInMemory()
        self.assertTrue(stage)

        scene = UsdPhysics.Scene.Define(stage, '/physicsScene')
        self.assertTrue(scene)

        xform = UsdGeom.Xform.Define(stage, "/xform")
        rbo_api = UsdPhysics.RigidBodyAPI.Apply(xform.GetPrim())

        cube = UsdGeom.Cube.Define(stage, "/xform/cube")
        collision_api = UsdPhysics.CollisionAPI.Apply(cube.GetPrim())

        custom_tokens = UsdPhysics.CustomUsdPhysicsTokens()
        simulation_owners = [scene.GetPrim().GetPrimPath()]
        ret_dict = UsdPhysics.LoadUsdPhysicsFromRange(stage, ["/"], [], 
                custom_tokens, simulation_owners)

        def check_ret_dict():
            scene_found = False
            rigid_body_found = False
            collision_found = False

            for key, value in ret_dict.items():
                prim_paths, descs = value
                if key == UsdPhysics.ObjectType.Scene:
                    scene_found = True
                elif key == UsdPhysics.ObjectType.RigidBody:
                    for prim_path, desc in zip(prim_paths, descs):
                        rigid_body_found = True
                elif key == UsdPhysics.ObjectType.CubeShape:
                    for prim_path, desc in zip(prim_paths, descs):
                        collision_found = True

            return scene_found, rigid_body_found, collision_found

        scene_found, rigid_body_found, collision_found = check_ret_dict()

        self.assertTrue(scene_found)
        self.assertTrue(not rigid_body_found)
        self.assertTrue(not collision_found)

        simulation_owners = [Sdf.Path()]
        ret_dict = UsdPhysics.LoadUsdPhysicsFromRange(stage, ["/"], [], 
                custom_tokens, simulation_owners)
        scene_found, rigid_body_found, collision_found = check_ret_dict()

        self.assertTrue(not scene_found)
        self.assertTrue(rigid_body_found)
        self.assertTrue(collision_found)

        simulation_owners = [scene.GetPrim().GetPrimPath()]
        rbo_api.GetSimulationOwnerRel().AddTarget(scene.GetPrim().GetPrimPath())
        ret_dict = UsdPhysics.LoadUsdPhysicsFromRange(stage, ["/"], [], 
                custom_tokens, simulation_owners)
        scene_found, rigid_body_found, collision_found = check_ret_dict()

        self.assertTrue(scene_found)
        self.assertTrue(rigid_body_found)
        self.assertTrue(collision_found)

        xform.GetPrim().RemoveAPI(UsdPhysics.RigidBodyAPI)
        simulation_owners = [Sdf.Path()]
        ret_dict = UsdPhysics.LoadUsdPhysicsFromRange(stage, ["/"], [], 
                custom_tokens, simulation_owners)
        scene_found, rigid_body_found, collision_found = check_ret_dict()

        self.assertTrue(not scene_found)
        self.assertTrue(not rigid_body_found)
        self.assertTrue(collision_found)

        simulation_owners = [scene.GetPrim().GetPrimPath()]
        ret_dict = UsdPhysics.LoadUsdPhysicsFromRange(stage, ["/"], [], 
                custom_tokens, simulation_owners)
        scene_found, rigid_body_found, collision_found = check_ret_dict()

        self.assertTrue(scene_found)
        self.assertTrue(not rigid_body_found)
        self.assertTrue(not collision_found)

        simulation_owners = [scene.GetPrim().GetPrimPath()]
        collision_api.GetSimulationOwnerRel().AddTarget(
            scene.GetPrim().GetPrimPath())
        ret_dict = UsdPhysics.LoadUsdPhysicsFromRange(stage, ["/"], [], 
                custom_tokens, simulation_owners)
        scene_found, rigid_body_found, collision_found = check_ret_dict()

        self.assertTrue(scene_found)
        self.assertTrue(not rigid_body_found)
        self.assertTrue(collision_found)

    def test_joint_simulation_owner_parse(self):
        stage = Usd.Stage.CreateInMemory()
        self.assertTrue(stage)

        scene = UsdPhysics.Scene.Define(stage, '/physicsScene')
        self.assertTrue(scene)

        xform_0 = UsdGeom.Xform.Define(stage, "/xform0")
        rbo_api_0 = UsdPhysics.RigidBodyAPI.Apply(xform_0.GetPrim())

        xform_1 = UsdGeom.Xform.Define(stage, "/xform1")
        rbo_api_1 = UsdPhysics.RigidBodyAPI.Apply(xform_1.GetPrim())

        fixed_joint = UsdPhysics.FixedJoint.Define(stage, "/fixedJoint")

        def check_ret_dict():
            scene_found = False
            rigid_body_found = False
            joint_found = False

            for key, value in ret_dict.items():
                prim_paths, descs = value
                if key == UsdPhysics.ObjectType.Scene:
                    scene_found = True
                elif key == UsdPhysics.ObjectType.RigidBody:
                    for prim_path, desc in zip(prim_paths, descs):
                        rigid_body_found = True
                elif key == UsdPhysics.ObjectType.FixedJoint:
                    for prim_path, desc in zip(prim_paths, descs):
                        joint_found = True

            return scene_found, rigid_body_found, joint_found

        test_cases = ["body0", "body1", "both"]
        for tc in test_cases:
            rbo_api_0.GetSimulationOwnerRel().ClearTargets(False)
            rbo_api_1.GetSimulationOwnerRel().ClearTargets(False)

            if tc == "body0":
                fixed_joint.GetBody0Rel().AddTarget(
                    xform_0.GetPrim().GetPrimPath())
            elif tc == "body1":
                fixed_joint.GetBody1Rel().AddTarget(
                    xform_1.GetPrim().GetPrimPath())
            elif tc == "both":
                fixed_joint.GetBody0Rel().AddTarget(
                    xform_0.GetPrim().GetPrimPath())
                fixed_joint.GetBody1Rel().AddTarget(
                    xform_1.GetPrim().GetPrimPath())

            custom_tokens = UsdPhysics.CustomUsdPhysicsTokens()
            simulation_owners = [scene.GetPrim().GetPrimPath()]
            ret_dict = UsdPhysics.LoadUsdPhysicsFromRange(stage, ["/"], [], 
                    custom_tokens, simulation_owners)

            scene_found, rigid_body_found, joint_found = check_ret_dict()

            self.assertTrue(scene_found)
            self.assertTrue(not rigid_body_found)
            self.assertTrue(not joint_found)

            simulation_owners = [Sdf.Path()]
            ret_dict = UsdPhysics.LoadUsdPhysicsFromRange(stage, ["/"], [], 
                    custom_tokens, simulation_owners)
            scene_found, rigid_body_found, joint_found = check_ret_dict()

            self.assertTrue(not scene_found)
            self.assertTrue(rigid_body_found)
            self.assertTrue(joint_found)

            rbo_api_0.GetSimulationOwnerRel().AddTarget(
                scene.GetPrim().GetPrimPath())
            rbo_api_1.GetSimulationOwnerRel().AddTarget(
                scene.GetPrim().GetPrimPath())
            simulation_owners = [scene.GetPrim().GetPrimPath()]
            ret_dict = UsdPhysics.LoadUsdPhysicsFromRange(stage, ["/"], [], 
                    custom_tokens, simulation_owners)
            scene_found, rigid_body_found, joint_found = check_ret_dict()

            self.assertTrue(scene_found)
            self.assertTrue(rigid_body_found)
            self.assertTrue(joint_found)

            rbo_api_0.GetSimulationOwnerRel().ClearTargets(False)            
            rbo_api_1.GetSimulationOwnerRel().AddTarget(
                scene.GetPrim().GetPrimPath())
            simulation_owners = [scene.GetPrim().GetPrimPath()]
            ret_dict = UsdPhysics.LoadUsdPhysicsFromRange(stage, ["/"], [], 
                    custom_tokens, simulation_owners)
            scene_found, rigid_body_found, joint_found = check_ret_dict()

            self.assertTrue(scene_found)
            self.assertTrue(rigid_body_found)
            self.assertTrue(not joint_found)

            rbo_api_0.GetSimulationOwnerRel().ClearTargets(False)            
            rbo_api_1.GetSimulationOwnerRel().AddTarget(
                scene.GetPrim().GetPrimPath())
            simulation_owners = [Sdf.Path(), scene.GetPrim().GetPrimPath()]
            ret_dict = UsdPhysics.LoadUsdPhysicsFromRange(stage, ["/"], [], 
                    custom_tokens, simulation_owners)
            scene_found, rigid_body_found, joint_found = check_ret_dict()

            self.assertTrue(scene_found)
            self.assertTrue(rigid_body_found)
            self.assertTrue(joint_found)

    def test_articulation_simulation_owner_parse(self):
        stage = Usd.Stage.CreateInMemory()
        self.assertTrue(stage)

        scene = UsdPhysics.Scene.Define(stage, '/physicsScene')
        self.assertTrue(scene)

        xform = UsdGeom.Xform.Define(stage, "/xform")
        UsdPhysics.ArticulationRootAPI.Apply(xform.GetPrim())

        cube_0 = UsdGeom.Cube.Define(stage, "/xform/cube0")
        rigid_body_api_0 = UsdPhysics.RigidBodyAPI.Apply(cube_0.GetPrim())

        cube_1 = UsdGeom.Cube.Define(stage, "/xform/cube1")
        rigid_body_api_1 = UsdPhysics.RigidBodyAPI.Apply(cube_1.GetPrim())

        cube_2 = UsdGeom.Cube.Define(stage, "/xform/cube2")
        rigid_body_api_2 = UsdPhysics.RigidBodyAPI.Apply(cube_2.GetPrim())

        revolute_joint_0 = UsdPhysics.RevoluteJoint.Define(
            stage, "/xform/revoluteJoint0")
        revolute_joint_0.GetBody0Rel().AddTarget(cube_0.GetPrim().GetPrimPath())
        revolute_joint_0.GetBody1Rel().AddTarget(cube_1.GetPrim().GetPrimPath())

        revolute_joint_1 = UsdPhysics.RevoluteJoint.Define(
            stage, "/xform/revoluteJoint1")
        revolute_joint_1.GetBody0Rel().AddTarget(cube_1.GetPrim().GetPrimPath())
        revolute_joint_1.GetBody1Rel().AddTarget(cube_2.GetPrim().GetPrimPath())

        custom_tokens = UsdPhysics.CustomUsdPhysicsTokens()
        simulation_owners = [scene.GetPrim().GetPrimPath()]
        ret_dict = UsdPhysics.LoadUsdPhysicsFromRange(stage, ["/"], [], 
                custom_tokens, simulation_owners)

        def check_ret_dict():
            scene_found = False
            rigid_body_found = False
            articulation_found = False
            joint_found = False

            for key, value in ret_dict.items():
                prim_paths, descs = value
                if key == UsdPhysics.ObjectType.Scene:
                    scene_found = True
                elif key == UsdPhysics.ObjectType.RigidBody:
                    for prim_path, desc in zip(prim_paths, descs):
                        rigid_body_found = True
                elif key == UsdPhysics.ObjectType.RevoluteJoint:
                    for prim_path, desc in zip(prim_paths, descs):
                        joint_found = True
                elif key == UsdPhysics.ObjectType.Articulation:
                    for prim_path, desc in zip(prim_paths, descs):
                        articulation_found = True

            return scene_found, rigid_body_found, joint_found, articulation_found

        scene_found, rigid_body_found, joint_found, articulation_found = \
            check_ret_dict()

        self.assertTrue(scene_found)
        self.assertTrue(not rigid_body_found)
        self.assertTrue(not joint_found)
        self.assertTrue(not articulation_found)

        simulation_owners = [Sdf.Path()]
        ret_dict = UsdPhysics.LoadUsdPhysicsFromRange(stage, ["/"], [], 
                custom_tokens, simulation_owners)
        scene_found, rigid_body_found, joint_found, articulation_found = \
            check_ret_dict()

        self.assertTrue(not scene_found)
        self.assertTrue(rigid_body_found)
        self.assertTrue(joint_found)
        self.assertTrue(articulation_found)

        simulation_owners = [scene.GetPrim().GetPrimPath()]
        rigid_body_api_0.GetSimulationOwnerRel().AddTarget(
            scene.GetPrim().GetPrimPath())
        rigid_body_api_1.GetSimulationOwnerRel().AddTarget(
            scene.GetPrim().GetPrimPath())
        ret_dict = UsdPhysics.LoadUsdPhysicsFromRange(stage, ["/"], [], 
                custom_tokens, simulation_owners)
        scene_found, rigid_body_found, joint_found, articulation_found = \
            check_ret_dict()

        self.assertTrue(scene_found)
        self.assertTrue(rigid_body_found)
        self.assertTrue(joint_found)
        self.assertTrue(not articulation_found)

        simulation_owners = [scene.GetPrim().GetPrimPath()]
        rigid_body_api_0.GetSimulationOwnerRel().AddTarget(
            scene.GetPrim().GetPrimPath())
        rigid_body_api_1.GetSimulationOwnerRel().AddTarget(
            scene.GetPrim().GetPrimPath())
        rigid_body_api_2.GetSimulationOwnerRel().AddTarget(
            scene.GetPrim().GetPrimPath())
        ret_dict = UsdPhysics.LoadUsdPhysicsFromRange(stage, ["/"], [], 
                custom_tokens, simulation_owners)
        scene_found, rigid_body_found, joint_found, articulation_found = \
            check_ret_dict()

        self.assertTrue(scene_found)
        self.assertTrue(rigid_body_found)
        self.assertTrue(joint_found)
        self.assertTrue(articulation_found)


if __name__ == "__main__":
    unittest.main()
