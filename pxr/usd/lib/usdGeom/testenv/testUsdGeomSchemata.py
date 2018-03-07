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
from pxr import Sdf, Usd, UsdGeom, Vt, Gf

class TestUsdGeomSchemata(unittest.TestCase):
    def test_Basic(self):
        l = Sdf.Layer.CreateAnonymous()
        stage = Usd.Stage.Open(l.identifier)

        p = stage.DefinePrim("/Mesh", "Mesh")
        self.assertTrue(p)

        mesh = UsdGeom.Mesh(p)
        self.assertTrue(mesh)
        self.assertTrue(mesh.GetPrim())
        self.assertTrue(not mesh.GetPointsAttr().Get(1))
        self.assertEqual(p.GetTypeName(), mesh.GetSchemaClassPrimDefinition().typeName)

        #
        # Make sure uniform access behaves as expected.
        #
        ori = p.GetAttribute("orientation")

        # The generic orientation attribute should be automatically defined because
        # it is a registered attribute of a well known schema.  However, it's not
        # yet authored at the current edit target.
        self.assertTrue(ori.IsDefined())
        self.assertTrue(not ori.IsAuthoredAt(ori.GetStage().GetEditTarget()))
        # Author a value, and check that it's still defined, and now is in fact
        # authored at the current edit target.
        ori.Set(UsdGeom.Tokens.leftHanded)
        self.assertTrue(ori.IsDefined())
        self.assertTrue(ori.IsAuthoredAt(ori.GetStage().GetEditTarget()))
        mesh.GetOrientationAttr().Set(UsdGeom.Tokens.rightHanded, 10)

        # "leftHanded" should have been authored at Usd.TimeCode.Default, so reading the
        # attribute at Default should return lh, not rh.
        self.assertEqual(ori.Get(), UsdGeom.Tokens.leftHanded)

        # The value "rightHanded" was set at t=10, so reading *any* time should
        # return "rightHanded"
        self.assertEqual(ori.Get(9.9),  UsdGeom.Tokens.rightHanded)
        self.assertEqual(ori.Get(10),   UsdGeom.Tokens.rightHanded)
        self.assertEqual(ori.Get(10.1), UsdGeom.Tokens.rightHanded)
        self.assertEqual(ori.Get(11),   UsdGeom.Tokens.rightHanded)

        #
        # Attribute name sanity check. We expect the names returned by the schema
        # to match the names returned via the generic API.
        #
        self.assertTrue(len(mesh.GetSchemaAttributeNames()) > 0)
        self.assertNotEqual(mesh.GetSchemaAttributeNames(True), mesh.GetSchemaAttributeNames(False))

        for n in mesh.GetSchemaAttributeNames():
            # apiName overrides
            if n == "primvars:displayColor":
                n = "displayColor"
            elif n == "primvars:displayOpacity":
                n = "displayOpacity"

            name = n[0].upper() + n[1:]
            self.assertTrue(("Get" + name + "Attr") in dir(mesh), 
                            ("Get" + name + "Attr() not found in: " + str(dir(mesh))))

    def test_IsA(self):

        # Author Scene and Compose Stage

        l = Sdf.Layer.CreateAnonymous()
        stage = Usd.Stage.Open(l.identifier)

        # For every prim schema type in this module, validate that:
        # 1. We can define a prim of its type
        # 2. Its type and inheritance matches our expectations
        # 3. At least one of its builtin properties is available and defined

        # BasisCurves Tests

        schema = UsdGeom.BasisCurves.Define(stage, "/BasisCurves")
        self.assertTrue(schema)
        prim = schema.GetPrim()
        self.assertFalse(prim.IsA(UsdGeom.Mesh))       # BasisCurves is not a Mesh
        self.assertTrue(prim.IsA(UsdGeom.Xformable))   # BasisCurves is a Xformable
        self.assertFalse(prim.IsA(UsdGeom.Cylinder))   # BasisCurves is not a Cylinder
        self.assertTrue(schema.GetBasisAttr())

        # Camera Tests

        schema = UsdGeom.Camera.Define(stage, "/Camera")
        self.assertTrue(schema)
        prim = schema.GetPrim()
        self.assertFalse(prim.IsA(UsdGeom.Mesh))       # Camera is not a Mesh
        self.assertTrue(prim.IsA(UsdGeom.Xformable))   # Camera is a Xformable
        self.assertFalse(prim.IsA(UsdGeom.Cylinder))   # Camera is not a Cylinder
        self.assertTrue(schema.GetFocalLengthAttr())

        # Capsule Tests

        schema = UsdGeom.Capsule.Define(stage, "/Capsule")
        self.assertTrue(schema)
        prim = schema.GetPrim()
        self.assertFalse(prim.IsA(UsdGeom.Mesh))       # Capsule is not a Mesh
        self.assertTrue(prim.IsA(UsdGeom.Xformable))   # Capsule is a Xformable
        self.assertFalse(prim.IsA(UsdGeom.Cylinder))   # Capsule is not a Cylinder
        self.assertTrue(schema.GetAxisAttr())

        # Cone Tests

        schema = UsdGeom.Cone.Define(stage, "/Cone")
        self.assertTrue(schema)
        prim = schema.GetPrim()
        self.assertFalse(prim.IsA(UsdGeom.Mesh))       # Cone is not a Mesh
        self.assertTrue(prim.IsA(UsdGeom.Xformable))   # Cone is a Xformable
        self.assertFalse(prim.IsA(UsdGeom.Cylinder))   # Cone is not a Cylinder
        self.assertTrue(schema.GetAxisAttr())

        # Cube Tests

        schema = UsdGeom.Cube.Define(stage, "/Cube")
        self.assertTrue(schema)
        prim = schema.GetPrim()
        self.assertFalse(prim.IsA(UsdGeom.Mesh))       # Cube is not a Mesh
        self.assertTrue(prim.IsA(UsdGeom.Xformable))   # Cube is a Xformable
        self.assertFalse(prim.IsA(UsdGeom.Cylinder))   # Cube is not a Cylinder
        self.assertTrue(schema.GetSizeAttr())

        # Cylinder Tests

        schema = UsdGeom.Cylinder.Define(stage, "/Cylinder")
        self.assertTrue(schema)
        prim = schema.GetPrim()
        self.assertFalse(prim.IsA(UsdGeom.Mesh))       # Cylinder is not a Mesh
        self.assertTrue(prim.IsA(UsdGeom.Xformable))   # Cylinder is a Xformable
        self.assertTrue(prim.IsA(UsdGeom.Cylinder))    # Cylinder is a Cylinder
        self.assertTrue(schema.GetAxisAttr())

        # Mesh Tests

        schema = UsdGeom.Mesh.Define(stage, "/Mesh")
        self.assertTrue(schema)
        prim = schema.GetPrim()
        self.assertTrue(prim.IsA(UsdGeom.Mesh))        # Mesh is a Mesh
        self.assertTrue(prim.IsA(UsdGeom.Xformable))   # Mesh is a XFormable
        self.assertFalse(prim.IsA(UsdGeom.Cylinder))   # Mesh is not a Cylinder
        self.assertTrue(schema.GetFaceVertexCountsAttr())

        # NurbsCurves Tests

        schema = UsdGeom.NurbsCurves.Define(stage, "/NurbsCurves")
        self.assertTrue(schema)
        prim = schema.GetPrim()
        self.assertFalse(prim.IsA(UsdGeom.Mesh))       # NurbsCurves is not a Mesh
        self.assertTrue(prim.IsA(UsdGeom.Xformable))   # NurbsCurves is a Xformable
        self.assertFalse(prim.IsA(UsdGeom.Cylinder))   # NurbsCurves is not a Cylinder
        self.assertTrue(schema.GetKnotsAttr())

        # NurbsPatch Tests

        schema = UsdGeom.NurbsPatch.Define(stage, "/NurbsPatch")
        self.assertTrue(schema)
        prim = schema.GetPrim()
        self.assertFalse(prim.IsA(UsdGeom.Mesh))       # NurbsPatch is not a Mesh
        self.assertTrue(prim.IsA(UsdGeom.Xformable))   # NurbsPatch is a Xformable
        self.assertFalse(prim.IsA(UsdGeom.Cylinder))   # NurbsPatch is not a Cylinder
        self.assertTrue(schema.GetUKnotsAttr())

        # Points Tests

        schema = UsdGeom.Points.Define(stage, "/Points")
        self.assertTrue(schema)
        prim = schema.GetPrim()
        self.assertFalse(prim.IsA(UsdGeom.Mesh))       # Points is not a Mesh
        self.assertTrue(prim.IsA(UsdGeom.Xformable))   # Points is a Xformable
        self.assertFalse(prim.IsA(UsdGeom.Cylinder))   # Points is not a Cylinder
        self.assertTrue(schema.GetWidthsAttr())

        # Scope Tests

        schema = UsdGeom.Scope.Define(stage, "/Scope")
        self.assertTrue(schema)
        prim = schema.GetPrim()
        self.assertFalse(prim.IsA(UsdGeom.Mesh))       # Scope is not a Mesh
        self.assertFalse(prim.IsA(UsdGeom.Xformable))  # Scope is not a Xformable
        self.assertFalse(prim.IsA(UsdGeom.Cylinder))   # Scope is not a Cylinder
        # Scope has no builtins!

        # Sphere Tests

        schema = UsdGeom.Sphere.Define(stage, "/Sphere")
        self.assertTrue(schema)
        prim = schema.GetPrim()
        self.assertFalse(prim.IsA(UsdGeom.Mesh))       # Sphere is not a Mesh
        self.assertTrue(prim.IsA(UsdGeom.Xformable))   # Sphere is a Xformable
        self.assertFalse(prim.IsA(UsdGeom.Cylinder))   # Sphere is not a Cylinder
        self.assertTrue(schema.GetRadiusAttr())

        # Xform Tests

        schema = UsdGeom.Xform.Define(stage, "/Xform")
        self.assertTrue(schema)
        prim = schema.GetPrim()
        self.assertFalse(prim.IsA(UsdGeom.Mesh))       # Xform is not a Mesh
        self.assertTrue(prim.IsA(UsdGeom.Xformable))   # Xform is a Xformable
        self.assertFalse(prim.IsA(UsdGeom.Cylinder))   # Xform is not a Cylinder
        self.assertTrue(schema.GetXformOpOrderAttr())


    def test_Fallbacks(self):
        # Author Scene and Compose Stage

        stage = Usd.Stage.CreateInMemory()

        # Xformable Tests

        identity = Gf.Matrix4d(1)
        origin = Gf.Vec3f(0, 0, 0)

        xform = UsdGeom.Xform.Define(stage, "/Xform")  # direct subclass
        xformOpOrder = xform.GetXformOpOrderAttr()
        self.assertFalse(xformOpOrder.HasAuthoredValueOpinion())
        # xformOpOrder has no fallback value
        self.assertEqual(xformOpOrder.Get(), None)
        self.assertFalse(xformOpOrder.HasFallbackValue())

        # Try authoring and reverting...
        xformOpOrderAttr = xform.GetPrim().GetAttribute(UsdGeom.Tokens.xformOpOrder)
        self.assertTrue(xformOpOrderAttr)
        self.assertEqual(xformOpOrderAttr.Get(), None)

        opOrderVal = ["xformOp:transform"]
        self.assertTrue(xformOpOrderAttr.Set(opOrderVal))
        self.assertTrue(xformOpOrderAttr.HasAuthoredValueOpinion())

        self.assertNotEqual(xformOpOrderAttr.Get(), None)
        self.assertTrue(xformOpOrderAttr.Clear())
        self.assertFalse(xformOpOrderAttr.HasAuthoredValueOpinion())
        self.assertEqual(xformOpOrderAttr.Get(), None)
        self.assertFalse(xformOpOrder.HasFallbackValue())

        mesh = UsdGeom.Mesh.Define(stage, "/Mesh")  # multiple ancestor hops

        # PointBased and Curves
        curves = UsdGeom.BasisCurves.Define(stage, "/Curves")
        self.assertEqual(curves.GetNormalsInterpolation(), UsdGeom.Tokens.varying)
        self.assertEqual(curves.GetWidthsInterpolation(), UsdGeom.Tokens.varying)

        # Before we go, test that CreateXXXAttr performs as we expect in various
        # scenarios
        # Number 1: Sparse and non-sparse authoring on def'd prim
        mesh.CreateDoubleSidedAttr(False, True)
        self.assertFalse(mesh.GetDoubleSidedAttr().HasAuthoredValueOpinion())
        mesh.CreateDoubleSidedAttr(False, False)
        self.assertTrue(mesh.GetDoubleSidedAttr().HasAuthoredValueOpinion())

        # Number 2: Sparse authoring demotes to dense for non-defed prim
        overMesh = UsdGeom.Mesh(stage.OverridePrim('/overMesh'))
        overMesh.CreateDoubleSidedAttr(False, True)
        self.assertTrue(overMesh.GetDoubleSidedAttr().HasAuthoredValueOpinion())
        self.assertEqual(overMesh.GetDoubleSidedAttr().Get(), False)
        overMesh.CreateDoubleSidedAttr(True, True)
        self.assertEqual(overMesh.GetDoubleSidedAttr().Get(), True)
        # make it a defined mesh, and sanity check it still evals the same
        mesh2 = UsdGeom.Mesh.Define(stage, "/overMesh")
        self.assertEqual(overMesh.GetDoubleSidedAttr().Get(), True)

        # Check querying of fallback values.
        sphere = UsdGeom.Sphere.Define(stage, "/Sphere")
        radius = sphere.GetRadiusAttr()
        self.assertTrue(radius.HasFallbackValue())

        radiusQuery = Usd.AttributeQuery(radius)
        self.assertTrue(radiusQuery.HasFallbackValue())

    def test_DefineSchema(self):
        s = Usd.Stage.CreateInMemory()
        parent = s.OverridePrim('/parent')
        self.assertTrue(parent)
        # Make a subscope.
        scope = UsdGeom.Scope.Define(s, '/parent/subscope')
        self.assertTrue(scope)
        # Assert that a simple find or create gives us the scope back.
        self.assertTrue(s.OverridePrim('/parent/subscope'))
        self.assertEqual(s.OverridePrim('/parent/subscope'), scope.GetPrim())
        # Try to make a mesh at subscope's path.  This transforms the scope into a
        # mesh, since Define() always authors typeName.
        mesh = UsdGeom.Mesh.Define(s, '/parent/subscope')
        self.assertTrue(mesh)
        self.assertTrue(not scope)
        # Make a mesh at a different path, should work.
        mesh = UsdGeom.Mesh.Define(s, '/parent/mesh')
        self.assertTrue(mesh)

    def test_BasicMetadataCases(self):
        s = Usd.Stage.CreateInMemory()
        spherePrim = UsdGeom.Sphere.Define(s, '/sphere').GetPrim()
        radius = spherePrim.GetAttribute('radius')
        self.assertTrue(radius.HasMetadata('custom'))
        self.assertTrue(radius.HasMetadata('typeName'))
        self.assertTrue(radius.HasMetadata('variability'))
        self.assertTrue(radius.IsDefined())
        self.assertTrue(not radius.IsCustom())
        self.assertEqual(radius.GetTypeName(), 'double')
        allMetadata = radius.GetAllMetadata()
        self.assertEqual(allMetadata['typeName'], 'double')
        self.assertEqual(allMetadata['variability'], Sdf.VariabilityVarying)
        self.assertEqual(allMetadata['custom'], False)
        # Author a custom property spec.
        layer = s.GetRootLayer()
        sphereSpec = layer.GetPrimAtPath('/sphere')
        radiusSpec = Sdf.AttributeSpec(
            sphereSpec, 'radius', Sdf.ValueTypeNames.Double,
            variability=Sdf.VariabilityUniform, declaresCustom=True)
        self.assertTrue(radiusSpec.custom)
        self.assertEqual(radiusSpec.variability, Sdf.VariabilityUniform)
        # Definition should win.
        self.assertTrue(not radius.IsCustom())
        self.assertEqual(radius.GetVariability(), Sdf.VariabilityVarying)
        allMetadata = radius.GetAllMetadata()
        self.assertEqual(allMetadata['typeName'], 'double')
        self.assertEqual(allMetadata['variability'], Sdf.VariabilityVarying)
        self.assertEqual(allMetadata['custom'], False)
        # List fields on 'visibility' attribute -- should include 'allowedTokens',
        # provided by the property definition.
        visibility = spherePrim.GetAttribute('visibility')
        self.assertTrue(visibility.IsDefined())
        self.assertTrue('allowedTokens' in visibility.GetAllMetadata())

        # Assert that attribute fallback values are returned for builtin attributes.
        do = spherePrim.GetAttribute('primvars:displayOpacity')
        self.assertTrue(do.IsDefined())
        self.assertTrue(do.Get() is None)

    def test_Camera(self):
        from pxr import Gf

        stage = Usd.Stage.CreateInMemory()

        camera = UsdGeom.Camera.Define(stage, "/Camera")

        self.assertTrue(camera.GetPrim().IsA(UsdGeom.Xformable)) # Camera is Xformable

        self.assertEqual(camera.GetProjectionAttr().Get(), 'perspective')
        camera.GetProjectionAttr().Set('orthographic')
        self.assertEqual(camera.GetProjectionAttr().Get(), 'orthographic')

        self.assertTrue(Gf.IsClose(camera.GetHorizontalApertureAttr().Get(),
                                   0.825 * 25.4, 1e-5))
        camera.GetHorizontalApertureAttr().Set(3.0)
        self.assertEqual(camera.GetHorizontalApertureAttr().Get(), 3.0)

        self.assertTrue(Gf.IsClose(camera.GetVerticalApertureAttr().Get(),
                                   0.602 * 25.4, 1e-5))
        camera.GetVerticalApertureAttr().Set(2.0)
        self.assertEqual(camera.GetVerticalApertureAttr().Get(), 2.0)

        self.assertEqual(camera.GetFocalLengthAttr().Get(), 50.0)
        camera.GetFocalLengthAttr().Set(35.0)
        self.assertTrue(Gf.IsClose(camera.GetFocalLengthAttr().Get(), 35.0, 1e-5))

        self.assertEqual(camera.GetClippingRangeAttr().Get(), Gf.Vec2f(1, 1000000))
        camera.GetClippingRangeAttr().Set(Gf.Vec2f(5, 10))
        self.assertTrue(Gf.IsClose(camera.GetClippingRangeAttr().Get(), 
                                   Gf.Vec2f(5, 10), 1e-5))

        self.assertEqual(camera.GetClippingPlanesAttr().Get(), Vt.Vec4fArray())

        cp = Vt.Vec4fArray([(1, 2, 3, 4), (8, 7, 6, 5)])
        camera.GetClippingPlanesAttr().Set(cp)
        self.assertEqual(camera.GetClippingPlanesAttr().Get(), cp)
        cp = Vt.Vec4fArray()
        camera.GetClippingPlanesAttr().Set(cp)
        self.assertEqual(camera.GetClippingPlanesAttr().Get(), cp)

        self.assertEqual(camera.GetFStopAttr().Get(), 0.0)
        camera.GetFStopAttr().Set(2.8)
        self.assertTrue(Gf.IsClose(camera.GetFStopAttr().Get(), 2.8, 1e-5))

        self.assertEqual(camera.GetFocusDistanceAttr().Get(), 0.0)
        camera.GetFocusDistanceAttr().Set(10.0)
        self.assertEqual(camera.GetFocusDistanceAttr().Get(), 10.0)

    def test_Points(self):
        stage = Usd.Stage.CreateInMemory()

        # Points Tests

        schema = UsdGeom.Points.Define(stage, "/Points")
        self.assertTrue(schema)
        
        # Test that id's roundtrip properly, for big numbers, and negative numbers
        ids = [8589934592, 1099511627776, 0, -42]
        schema.CreateIdsAttr(ids)
        resolvedIds = list(schema.GetIdsAttr().Get()) # convert VtArray to list
        self.assertEqual(ids, resolvedIds)
     

    def test_Bug111239(self):
        # This bug broke the schema registry for prims whose typenames authored in
        # scene description were their canonical C++ typenames, rather than their
        # alias under UsdSchemaBase.  For example, if you had a prim with typename
        # 'UsdGeomSphere' instead of 'Sphere', we would not find builtins for the
        # prim with typename 'UsdGeomSphere'.
        s = Usd.Stage.CreateInMemory()
        sphere = s.DefinePrim('/sphere', typeName='Sphere')
        usdGeomSphere = s.DefinePrim('/usdGeomSphere', typeName='UsdGeomSphere')
        self.assertTrue('radius' in [a.GetName() for a in sphere.GetAttributes()])
        self.assertTrue('radius' in [a.GetName() for a in usdGeomSphere.GetAttributes()])

    def test_ComputeExtent(self):

        from pxr import Gf

        # Create some simple test cases
        allPoints = [
            [(1, 1, 0)],    # Zero-Volume Extent Test
            [(0, 0, 0)],    # Simple Width Test
            [(-1, -1, -1), (1, 1, 1)],  # Multiple Width Test
            [(-1, -1, -1), (1, 1, 1)],  # Erroneous Widths/Points Test
            # Complex Test, Many Points/Widths
            [(3, -1, 5), (-1.5, 0, 3), (1, 3, -2), (2, 2, -4)],
        ]

        allWidths = [
            [0],            # Zero-Volume Extent Test
            [2],            # Simple Width Test
            [2, 4],         # Multiple Width Test
            [2, 4, 5],      # Erroneous Widths/Points Test
            [1, 2, 2, 1]    # Complex Test, Many Points/Widths
        ]

        pointBasedSolutions = [
            [(1, 1, 0), (1, 1, 0)],         # Zero-Volume Extent Test
            [(0, 0, 0), (0, 0, 0)],         # Simple Width Test
            [(-1, -1, -1), (1, 1, 1)],      # Multiple Width Test
            # Erroneous Widths/Points Test -> Ok For Point-Based
            [(-1, -1, -1), (1, 1, 1)],
            [(-1.5, -1, -4), (3, 3, 5)]     # Complex Test, Many Points/Widths
        ]

        pointsSolutions = [
            [(1, 1, 0), (1, 1, 0)],             # Zero-Volume Extent Test
            [(-1, -1, -1), (1, 1, 1)],          # Simple Width Test
            [(-2, -2, -2), (3, 3, 3)],          # Multiple Width Test
            # Erroneous Widths/Points Test -> Returns None
            None,
            [(-2.5, -1.5, -4.5), (3.5, 4, 5.5)] # Complex Test, Many Points/Widths
        ]

        # Perform the correctness tests for PointBased and Points

        # Test for empty points prims
        emptyPoints = []
        extremeExtentArr = UsdGeom.PointBased.ComputeExtent(emptyPoints)

        # We need to map the contents of extremeExtentArr to floats from
        # num.float32s due to the way Gf.Vec3f is wrapped out
        # XXX: This is awful, it'd be nice to not do it
        extremeExtentRange = Gf.Range3f(Gf.Vec3f(*map(float, extremeExtentArr[0])),
                                        Gf.Vec3f(*map(float, extremeExtentArr[1])))
        self.assertTrue(extremeExtentRange.IsEmpty())

        # PointBased Test
        numDataSets = len(allPoints)
        for i in range(numDataSets):
            pointsData = allPoints[i]

            expectedExtent = pointBasedSolutions[i]
            actualExtent = UsdGeom.PointBased.ComputeExtent(pointsData)

            for a, b in zip(expectedExtent, actualExtent):
                self.assertTrue(Gf.IsClose(a, b, 1e-5))

        # Points Test
        for i in range(numDataSets):
            pointsData = allPoints[i]
            widthsData = allWidths[i]

            expectedExtent = pointsSolutions[i]
            actualExtent = UsdGeom.Points.ComputeExtent(pointsData, widthsData)
            if actualExtent is not None and expectedExtent is not None:
                for a, b in zip(expectedExtent, actualExtent):
                    self.assertTrue(Gf.IsClose(a, b, 1e-5))

            # Compute extent via generic UsdGeom.Boundable API
            s = Usd.Stage.CreateInMemory()
            pointsPrim = UsdGeom.Points.Define(s, "/Points")
            pointsPrim.CreatePointsAttr(pointsData)
            pointsPrim.CreateWidthsAttr(widthsData)

            actualExtent = UsdGeom.Boundable.ComputeExtentFromPlugins(
                pointsPrim, Usd.TimeCode.Default())
            
            if actualExtent is not None and expectedExtent is not None:
                for a, b in zip(expectedExtent, list(actualExtent)):
                    self.assertTrue(Gf.IsClose(a, b, 1e-5))

        # Mesh Test
        for i in range(numDataSets):
            pointsData = allPoints[i]

            expectedExtent = pointBasedSolutions[i]

            # Compute extent via generic UsdGeom.Boundable API.
            # UsdGeom.Mesh does not have its own compute extent function, so
            # it should fall back to the extent for PointBased prims.
            s = Usd.Stage.CreateInMemory()
            meshPrim = UsdGeom.Mesh.Define(s, "/Mesh")
            meshPrim.CreatePointsAttr(pointsData)

            actualExtent = UsdGeom.Boundable.ComputeExtentFromPlugins(
                meshPrim, Usd.TimeCode.Default())

            for a, b in zip(expectedExtent, actualExtent):
                self.assertTrue(Gf.IsClose(a, b, 1e-5))



        # Test UsdGeomCurves
        curvesPoints = [
            [(0,0,0), (1,1,1), (2,1,1), (3,0,0)],   # Test Curve with 1 width
            [(0,0,0), (1,1,1), (2,1,1), (3,0,0)],   # Test Curve with 2 widths
            [(0,0,0), (1,1,1), (2,1,1), (3,0,0)]    # Test Curve with no width
        ]

        curvesWidths = [
            [1],            # Test Curve with 1 width
            [.5, .1],       # Test Curve with 2 widths
            []              # Test Curve with no width
        ]

        curvesSolutions = [
            [(-.5,-.5,-.5), (3.5,1.5,1.5)],         # Test Curve with 1 width
            [(-.25,-.25,-.25), (3.25,1.25,1.25)],   # Test Curve with 2 widths (MAX)
            [(0,0,0), (3,1,1)],                     # Test Curve with no width
        ]

        # Perform the actual v. expected comparison
        numDataSets = len(curvesPoints)
        for i in range(numDataSets):
            pointsData = curvesPoints[i]
            widths = curvesWidths[i]

            expectedExtent = curvesSolutions[i]
            actualExtent = UsdGeom.Curves.ComputeExtent(pointsData, widths)

            for a, b in zip(expectedExtent, actualExtent):
                self.assertTrue(Gf.IsClose(a, b, 1e-5))

            # Compute extent via generic UsdGeom.Boundable API
            s = Usd.Stage.CreateInMemory()
            nurbsCurvesPrim = UsdGeom.NurbsCurves.Define(s, "/NurbsCurves")
            nurbsCurvesPrim.CreatePointsAttr(pointsData)
            nurbsCurvesPrim.CreateWidthsAttr(widths)

            actualExtent = UsdGeom.Boundable.ComputeExtentFromPlugins(
                nurbsCurvesPrim, Usd.TimeCode.Default())

            for a, b in zip(expectedExtent, actualExtent):
                self.assertTrue(Gf.IsClose(a, b, 1e-5))

            basisCurvesPrim = UsdGeom.BasisCurves.Define(s, "/BasisCurves")
            basisCurvesPrim.CreatePointsAttr(pointsData)
            basisCurvesPrim.CreateWidthsAttr(widths)

            actualExtent = UsdGeom.Boundable.ComputeExtentFromPlugins(
                basisCurvesPrim, Usd.TimeCode.Default())

            for a, b in zip(expectedExtent, actualExtent):
                self.assertTrue(Gf.IsClose(a, b, 1e-5))

    def test_TypeUsage(self):
        # Perform Type-Ness Checking for ComputeExtent
        pointsAsList = [(0, 0, 0), (1, 1, 1), (2, 2, 2)]
        pointsAsVec3fArr = Vt.Vec3fArray(pointsAsList);

        comp = UsdGeom.PointBased.ComputeExtent
        expectedExtent = comp(pointsAsVec3fArr)
        actualExtent = comp(pointsAsList)

        for a, b in zip(expectedExtent, actualExtent):
            self.assertTrue(Gf.IsClose(a, b, 1e-5))

    def test_Bug116593(self):
        from pxr import Gf

        s = Usd.Stage.CreateInMemory()
        prim = s.DefinePrim('/sphere', typeName='Sphere')

        # set with list of tuples
        vec = [(1,2,2),(12,3,3)]
        self.assertTrue(UsdGeom.ModelAPI(prim).SetExtentsHint(vec))
        self.assertEqual(UsdGeom.ModelAPI(prim).GetExtentsHint()[0], Gf.Vec3f(1,2,2))
        self.assertEqual(UsdGeom.ModelAPI(prim).GetExtentsHint()[1], Gf.Vec3f(12,3,3))

        # set with Gf vecs
        vec = [Gf.Vec3f(1,2,2), Gf.Vec3f(1,1,1)]
        self.assertTrue(UsdGeom.ModelAPI(prim).SetExtentsHint(vec))
        self.assertEqual(UsdGeom.ModelAPI(prim).GetExtentsHint()[0], Gf.Vec3f(1,2,2))
        self.assertEqual(UsdGeom.ModelAPI(prim).GetExtentsHint()[1], Gf.Vec3f(1,1,1))

    def test_Typed(self):
        from pxr import Tf
        xform = Tf.Type.FindByName("UsdGeomXform")
        imageable = Tf.Type.FindByName("UsdGeomImageable")
        geomModelAPI = Tf.Type.FindByName("UsdGeomModelAPI")

        self.assertTrue(Usd.SchemaRegistry.IsTyped(xform))
        self.assertTrue(Usd.SchemaRegistry.IsTyped(imageable))
        self.assertFalse(Usd.SchemaRegistry.IsTyped(geomModelAPI))    
        
    def test_Concrete(self):
        from pxr import Tf
        xform = Tf.Type.FindByName("UsdGeomXform")
        imageable = Tf.Type.FindByName("UsdGeomImageable")
        geomModelAPI = Tf.Type.FindByName("UsdGeomModelAPI")

        self.assertTrue(Usd.SchemaRegistry.IsConcrete(xform))
        self.assertFalse(Usd.SchemaRegistry.IsConcrete(imageable))
        self.assertFalse(Usd.SchemaRegistry.IsConcrete(geomModelAPI))

    def test_Apply(self):
        s = Usd.Stage.CreateInMemory('AppliedSchemas.usd')
        root = s.DefinePrim('/hello')
        self.assertEqual([], root.GetAppliedSchemas())

        # Check duplicates
        UsdGeom.MotionAPI.Apply(root)
        self.assertEqual(['MotionAPI'], root.GetAppliedSchemas())
        UsdGeom.MotionAPI.Apply(root)
        self.assertEqual(['MotionAPI'], root.GetAppliedSchemas())
        
        # Ensure duplicates aren't picked up
        UsdGeom.ModelAPI.Apply(root)
        self.assertEqual(['MotionAPI', 'GeomModelAPI'], root.GetAppliedSchemas())

    def test_IsA(self):
        from pxr import Usd, Tf
        s = Usd.Stage.CreateInMemory()
        spherePrim = s.DefinePrim('/sphere', typeName='Sphere')
        typelessPrim = s.DefinePrim('/regular')

        types = [Tf.Type.FindByName('UsdGeomSphere'),
                 Tf.Type.FindByName('UsdGeomGprim'),
                 Tf.Type.FindByName('UsdGeomBoundable'),
                 Tf.Type.FindByName('UsdGeomXformable'),
                 Tf.Type.FindByName('UsdGeomImageable'),
                 Tf.Type.FindByName('UsdTyped')]

        # Our sphere prim should return true on IsA queries for Sphere
        # and everything it inherits from. Our plain prim should return false 
        # for all of them.
        for t in types:
            self.assertTrue(spherePrim.IsA(t))
            self.assertFalse(typelessPrim.IsA(t))

    def test_HasAPI(self):
        from pxr import Usd, Tf
        s = Usd.Stage.CreateInMemory()
        prim = s.DefinePrim('/prim')

        types = [Tf.Type.FindByName('UsdGeomMotionAPI'),
                 Tf.Type.FindByName('UsdGeomModelAPI'),
                 Tf.Type.FindByName('UsdModelAPI')]

        # Check that no APIs have yet been applied
        for t in types:
            self.assertFalse(prim.HasAPI(t))

        # Apply our schemas to this prim
        UsdGeom.ModelAPI.Apply(prim)
        UsdGeom.MotionAPI.Apply(prim)

        # Note that were applying an ancestor type of UsdGeomModelAPI
        Usd.ModelAPI.Apply(prim)

        # Check that all our applied schemas show up
        for t in types:
            self.assertTrue(prim.HasAPI(t))

        # Check that we get an exception for unknown and non-API types
        with self.assertRaises(Tf.ErrorException):
            prim.HasAPI(Tf.Type.Unknown)
        
        with self.assertRaises(Tf.ErrorException):
            prim.HasAPI(Tf.Type.FindByName('UsdGeomXform'))

        with self.assertRaises(Tf.ErrorException):
            prim.HasAPI(Tf.Type.FindByName('UsdGeomImageable'))

if __name__ == "__main__":
    unittest.main()
