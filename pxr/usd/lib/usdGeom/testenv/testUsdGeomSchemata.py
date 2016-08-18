#!/pxrpythonsubst

import sys
from pxr import Sdf, Usd, UsdGeom, Vt
from Mentor.Runtime import AssertEqual,\
                           AssertNotEqual,\
                           AssertClose,\
                           AssertTrue,\
                           AssertFalse,\
                           SetAssertMode,\
                           MTR_EXIT_TEST

def BasicTest():
    l = Sdf.Layer.CreateAnonymous()
    stage = Usd.Stage.Open(l.identifier)

    p = stage.DefinePrim("/Mesh", "Mesh")
    assert p

    mesh = UsdGeom.Mesh(p)
    assert mesh
    assert mesh.GetPrim()
    assert not mesh.GetPointsAttr().Get(1)
    AssertEqual(p.GetTypeName(), mesh.GetSchemaClassPrimDefinition().typeName)

    #
    # Make sure uniform access behaves as expected.
    #
    ori = p.GetAttribute("orientation")

    # The generic orientation attribute should be automatically defined because
    # it is a registered attribute of a well known schema.  However, it's not
    # yet authored at the current edit target.
    assert ori.IsDefined()
    assert not ori.IsAuthoredAt(ori.GetStage().GetEditTarget())
    # Author a value, and check that it's still defined, and now is in fact
    # authored at the current edit target.
    ori.Set(UsdGeom.Tokens.leftHanded)
    assert ori.IsDefined()
    assert ori.IsAuthoredAt(ori.GetStage().GetEditTarget())
    mesh.GetOrientationAttr().Set(UsdGeom.Tokens.rightHanded, 10)

    # "leftHanded" should have been authored at Usd.TimeCode.Default, so reading the
    # attribute at Default should return lh, not rh.
    AssertEqual(ori.Get(), UsdGeom.Tokens.leftHanded)

    # The value "rightHanded" was set at t=10, so reading *any* time should
    # return "rightHanded"
    AssertEqual(ori.Get(9.9),  UsdGeom.Tokens.rightHanded)
    AssertEqual(ori.Get(10),   UsdGeom.Tokens.rightHanded)
    AssertEqual(ori.Get(10.1), UsdGeom.Tokens.rightHanded)
    AssertEqual(ori.Get(11),   UsdGeom.Tokens.rightHanded)

    #
    # Attribute name sanity check. We expect the names returned by the schema
    # to match the names returned via the generic API.
    #
    assert len(mesh.GetSchemaAttributeNames()) > 0
    AssertNotEqual(mesh.GetSchemaAttributeNames(True), mesh.GetSchemaAttributeNames(False))

    for n in mesh.GetSchemaAttributeNames():
        # apiName overrides
        if n == "primvars:displayColor":
            n = "displayColor"
        elif n == "primvars:displayOpacity":
            n = "displayOpacity"

        name = n[0].upper() + n[1:]
        assert ("Get" + name + "Attr") in dir(mesh), \
                ("Get" + name + "Attr() not found in: " + str(dir(mesh)))

def TestIsA():

    # Author Scene and Compose Stage

    l = Sdf.Layer.CreateAnonymous()
    stage = Usd.Stage.Open(l.identifier)

    # For every prim schema type in this module, validate that:
    # 1. We can define a prim of its type
    # 2. Its type and inheritance matches our expectations
    # 3. At least one of its builtin properties is available and defined

    # BasisCurves Tests

    schema = UsdGeom.BasisCurves.Define(stage, "/BasisCurves")
    AssertTrue(schema)
    prim = schema.GetPrim()
    AssertFalse(prim.IsA(UsdGeom.Mesh))       # BasisCurves is not a Mesh
    AssertTrue(prim.IsA(UsdGeom.Xformable))   # BasisCurves is a Xformable
    AssertFalse(prim.IsA(UsdGeom.Cylinder))   # BasisCurves is not a Cylinder
    AssertTrue(schema.GetBasisAttr())

    # Camera Tests

    schema = UsdGeom.Camera.Define(stage, "/Camera")
    AssertTrue(schema)
    prim = schema.GetPrim()
    AssertFalse(prim.IsA(UsdGeom.Mesh))       # Camera is not a Mesh
    AssertTrue(prim.IsA(UsdGeom.Xformable))   # Camera is a Xformable
    AssertFalse(prim.IsA(UsdGeom.Cylinder))   # Camera is not a Cylinder
    AssertTrue(schema.GetFocalLengthAttr())

    # Capsule Tests

    schema = UsdGeom.Capsule.Define(stage, "/Capsule")
    AssertTrue(schema)
    prim = schema.GetPrim()
    AssertFalse(prim.IsA(UsdGeom.Mesh))       # Capsule is not a Mesh
    AssertTrue(prim.IsA(UsdGeom.Xformable))   # Capsule is a Xformable
    AssertFalse(prim.IsA(UsdGeom.Cylinder))   # Capsule is not a Cylinder
    AssertTrue(schema.GetAxisAttr())

    # Cone Tests

    schema = UsdGeom.Cone.Define(stage, "/Cone")
    AssertTrue(schema)
    prim = schema.GetPrim()
    AssertFalse(prim.IsA(UsdGeom.Mesh))       # Cone is not a Mesh
    AssertTrue(prim.IsA(UsdGeom.Xformable))   # Cone is a Xformable
    AssertFalse(prim.IsA(UsdGeom.Cylinder))   # Cone is not a Cylinder
    AssertTrue(schema.GetAxisAttr())

    # Cube Tests

    schema = UsdGeom.Cube.Define(stage, "/Cube")
    AssertTrue(schema)
    prim = schema.GetPrim()
    AssertFalse(prim.IsA(UsdGeom.Mesh))       # Cube is not a Mesh
    AssertTrue(prim.IsA(UsdGeom.Xformable))   # Cube is a Xformable
    AssertFalse(prim.IsA(UsdGeom.Cylinder))   # Cube is not a Cylinder
    AssertTrue(schema.GetSizeAttr())

    # Cylinder Tests

    schema = UsdGeom.Cylinder.Define(stage, "/Cylinder")
    AssertTrue(schema)
    prim = schema.GetPrim()
    AssertFalse(prim.IsA(UsdGeom.Mesh))       # Cylinder is not a Mesh
    AssertTrue(prim.IsA(UsdGeom.Xformable))   # Cylinder is a Xformable
    AssertTrue(prim.IsA(UsdGeom.Cylinder))    # Cylinder is a Cylinder
    AssertTrue(schema.GetAxisAttr())

    # Mesh Tests

    schema = UsdGeom.Mesh.Define(stage, "/Mesh")
    AssertTrue(schema)
    prim = schema.GetPrim()
    AssertTrue(prim.IsA(UsdGeom.Mesh))        # Mesh is a Mesh
    AssertTrue(prim.IsA(UsdGeom.Xformable))   # Mesh is a XFormable
    AssertFalse(prim.IsA(UsdGeom.Cylinder))   # Mesh is not a Cylinder
    AssertTrue(schema.GetFaceVertexCountsAttr())

    # NurbsCurves Tests

    schema = UsdGeom.NurbsCurves.Define(stage, "/NurbsCurves")
    AssertTrue(schema)
    prim = schema.GetPrim()
    AssertFalse(prim.IsA(UsdGeom.Mesh))       # NurbsCurves is not a Mesh
    AssertTrue(prim.IsA(UsdGeom.Xformable))   # NurbsCurves is a Xformable
    AssertFalse(prim.IsA(UsdGeom.Cylinder))   # NurbsCurves is not a Cylinder
    AssertTrue(schema.GetKnotsAttr())

    # NurbsPatch Tests

    schema = UsdGeom.NurbsPatch.Define(stage, "/NurbsPatch")
    AssertTrue(schema)
    prim = schema.GetPrim()
    AssertFalse(prim.IsA(UsdGeom.Mesh))       # NurbsPatch is not a Mesh
    AssertTrue(prim.IsA(UsdGeom.Xformable))   # NurbsPatch is a Xformable
    AssertFalse(prim.IsA(UsdGeom.Cylinder))   # NurbsPatch is not a Cylinder
    AssertTrue(schema.GetUKnotsAttr())

    # Points Tests

    schema = UsdGeom.Points.Define(stage, "/Points")
    AssertTrue(schema)
    prim = schema.GetPrim()
    AssertFalse(prim.IsA(UsdGeom.Mesh))       # Points is not a Mesh
    AssertTrue(prim.IsA(UsdGeom.Xformable))   # Points is a Xformable
    AssertFalse(prim.IsA(UsdGeom.Cylinder))   # Points is not a Cylinder
    AssertTrue(schema.GetWidthsAttr())

    # Scope Tests

    schema = UsdGeom.Scope.Define(stage, "/Scope")
    AssertTrue(schema)
    prim = schema.GetPrim()
    AssertFalse(prim.IsA(UsdGeom.Mesh))       # Scope is not a Mesh
    AssertFalse(prim.IsA(UsdGeom.Xformable))  # Scope is not a Xformable
    AssertFalse(prim.IsA(UsdGeom.Cylinder))   # Scope is not a Cylinder
    # Scope has no builtins!

    # Sphere Tests

    schema = UsdGeom.Sphere.Define(stage, "/Sphere")
    AssertTrue(schema)
    prim = schema.GetPrim()
    AssertFalse(prim.IsA(UsdGeom.Mesh))       # Sphere is not a Mesh
    AssertTrue(prim.IsA(UsdGeom.Xformable))   # Sphere is a Xformable
    AssertFalse(prim.IsA(UsdGeom.Cylinder))   # Sphere is not a Cylinder
    AssertTrue(schema.GetRadiusAttr())

    # Xform Tests

    schema = UsdGeom.Xform.Define(stage, "/Xform")
    AssertTrue(schema)
    prim = schema.GetPrim()
    AssertFalse(prim.IsA(UsdGeom.Mesh))       # Xform is not a Mesh
    AssertTrue(prim.IsA(UsdGeom.Xformable))   # Xform is a Xformable
    AssertFalse(prim.IsA(UsdGeom.Cylinder))   # Xform is not a Cylinder
    AssertTrue(schema.GetXformOpOrderAttr())


def TestFallbacks():
    # Author Scene and Compose Stage

    stage = Usd.Stage.CreateInMemory()

    # Xformable Tests

    from pxr import Gf

    identity = Gf.Matrix4d(1)
    origin = Gf.Vec3f(0, 0, 0)

    xform = UsdGeom.Xform.Define(stage, "/Xform")  # direct subclass
    xformOpOrder = xform.GetXformOpOrderAttr()
    AssertFalse(xformOpOrder.HasAuthoredValueOpinion())
    # xformOpOrder has no fallback value
    AssertEqual(xformOpOrder.Get(), None)
    AssertFalse(xformOpOrder.HasFallbackValue())

    # Try authoring and reverting...
    xformOpOrderAttr = xform.GetPrim().GetAttribute(UsdGeom.Tokens.xformOpOrder)
    AssertTrue(xformOpOrderAttr)
    AssertEqual(xformOpOrderAttr.Get(), None)

    opOrderVal = ["xformOp:transform"]
    AssertTrue(xformOpOrderAttr.Set(opOrderVal))
    AssertTrue(xformOpOrderAttr.HasAuthoredValueOpinion())

    AssertNotEqual(xformOpOrderAttr.Get(), None)
    AssertTrue(xformOpOrderAttr.Clear())
    AssertFalse(xformOpOrderAttr.HasAuthoredValueOpinion())
    AssertEqual(xformOpOrderAttr.Get(), None)
    AssertFalse(xformOpOrder.HasFallbackValue())

    mesh = UsdGeom.Mesh.Define(stage, "/Mesh")  # multiple ancestor hops

    # PointBased and Curves
    curves = UsdGeom.BasisCurves.Define(stage, "/Curves")
    AssertEqual(curves.GetNormalsInterpolation(), UsdGeom.Tokens.varying)
    AssertEqual(curves.GetWidthsInterpolation(), UsdGeom.Tokens.varying)

    # Before we go, test that CreateXXXAttr performs as we expect in various
    # scenarios
    # Number 1: Sparse and non-sparse authoring on def'd prim
    mesh.CreateDoubleSidedAttr(False, True)
    AssertFalse(mesh.GetDoubleSidedAttr().HasAuthoredValueOpinion())
    mesh.CreateDoubleSidedAttr(False, False)
    AssertTrue(mesh.GetDoubleSidedAttr().HasAuthoredValueOpinion())

    # Number 2: Sparse authoring demotes to dense for non-defed prim
    overMesh = UsdGeom.Mesh(stage.OverridePrim('/overMesh'))
    overMesh.CreateDoubleSidedAttr(False, True)
    AssertTrue(overMesh.GetDoubleSidedAttr().HasAuthoredValueOpinion())
    AssertEqual(overMesh.GetDoubleSidedAttr().Get(), False)
    overMesh.CreateDoubleSidedAttr(True, True)
    AssertEqual(overMesh.GetDoubleSidedAttr().Get(), True)
    # make it a defined mesh, and sanity check it still evals the same
    mesh2 = UsdGeom.Mesh.Define(stage, "/overMesh")
    AssertEqual(overMesh.GetDoubleSidedAttr().Get(), True)

    # Check querying of fallback values.
    sphere = UsdGeom.Sphere.Define(stage, "/Sphere")
    radius = sphere.GetRadiusAttr()
    AssertTrue(radius.HasFallbackValue())

    radiusQuery = Usd.AttributeQuery(radius)
    AssertTrue(radius.HasFallbackValue())


def DefineSchema():
    s = Usd.Stage.CreateInMemory()
    parent = s.OverridePrim('/parent')
    assert parent
    # Make a subscope.
    scope = UsdGeom.Scope.Define(s, '/parent/subscope')
    assert scope
    # Assert that a simple find or create gives us the scope back.
    assert s.OverridePrim('/parent/subscope')
    assert s.OverridePrim('/parent/subscope') == scope.GetPrim()
    # Try to make a mesh at subscope's path.  This transforms the scope into a
    # mesh, since Define() always authors typeName.
    mesh = UsdGeom.Mesh.Define(s, '/parent/subscope')
    assert mesh
    assert not scope
    # Make a mesh at a different path, should work.
    mesh = UsdGeom.Mesh.Define(s, '/parent/mesh')
    assert mesh


def BasicMetadataCases():
    s = Usd.Stage.CreateInMemory()
    spherePrim = UsdGeom.Sphere.Define(s, '/sphere').GetPrim()
    radius = spherePrim.GetAttribute('radius')
    assert radius.HasMetadata('custom')
    assert radius.HasMetadata('typeName')
    assert radius.HasMetadata('variability')
    assert radius.IsDefined()
    assert not radius.IsCustom()
    assert radius.GetTypeName() == 'double'
    allMetadata = radius.GetAllMetadata()
    assert allMetadata['typeName'] == 'double'
    assert allMetadata['variability'] == Sdf.VariabilityVarying
    assert allMetadata['custom'] == False
    # Author a custom property spec.
    layer = s.GetRootLayer()
    sphereSpec = layer.GetPrimAtPath('/sphere')
    radiusSpec = Sdf.AttributeSpec(
        sphereSpec, 'radius', Sdf.ValueTypeNames.Double,
        variability=Sdf.VariabilityUniform, declaresCustom=True)
    assert radiusSpec.custom
    assert radiusSpec.variability == Sdf.VariabilityUniform
    # Definition should win.
    assert not radius.IsCustom()
    assert radius.GetVariability() == Sdf.VariabilityVarying
    allMetadata = radius.GetAllMetadata()
    assert allMetadata['typeName'] == 'double'
    assert allMetadata['variability'] == Sdf.VariabilityVarying
    assert allMetadata['custom'] == False
    # List fields on 'visibility' attribute -- should include 'allowedTokens',
    # provided by the property definition.
    visibility = spherePrim.GetAttribute('visibility')
    assert visibility.IsDefined()
    assert 'allowedTokens' in visibility.GetAllMetadata()

    # Assert that attribute fallback values are returned for builtin attributes.
    do = spherePrim.GetAttribute('primvars:displayOpacity')
    assert do.IsDefined()
    assert do.Get() is None

def TestCamera():
    from pxr import Gf

    stage = Usd.Stage.CreateInMemory()

    camera = UsdGeom.Camera.Define(stage, "/Camera")

    AssertTrue(camera.GetPrim().IsA(UsdGeom.Xformable)) # Camera is Xformable

    AssertEqual(camera.GetProjectionAttr().Get(), 'perspective')
    camera.GetProjectionAttr().Set('orthographic')
    AssertEqual(camera.GetProjectionAttr().Get(), 'orthographic')

    AssertClose(camera.GetHorizontalApertureAttr().Get(), 0.825 * 25.4)
    camera.GetHorizontalApertureAttr().Set(3.0)
    AssertEqual(camera.GetHorizontalApertureAttr().Get(), 3.0)

    AssertClose(camera.GetVerticalApertureAttr().Get(), 0.602 * 25.4)
    camera.GetVerticalApertureAttr().Set(2.0)
    AssertEqual(camera.GetVerticalApertureAttr().Get(), 2.0)

    AssertEqual(camera.GetFocalLengthAttr().Get(), 50.0)
    camera.GetFocalLengthAttr().Set(35.0)
    AssertClose(camera.GetFocalLengthAttr().Get(), 35.0)

    AssertEqual(camera.GetClippingRangeAttr().Get(), Gf.Vec2f(1, 1000000))
    camera.GetClippingRangeAttr().Set(Gf.Vec2f(5, 10))
    AssertClose(camera.GetClippingRangeAttr().Get(), Gf.Vec2f(5, 10))

    AssertEqual(camera.GetClippingPlanesAttr().Get(), Vt.Vec4fArray())

    cp = Vt.Vec4fArray([(1, 2, 3, 4), (8, 7, 6, 5)])
    camera.GetClippingPlanesAttr().Set(cp)
    AssertEqual(camera.GetClippingPlanesAttr().Get(), cp)
    cp = Vt.Vec4fArray()
    camera.GetClippingPlanesAttr().Set(cp)
    AssertEqual(camera.GetClippingPlanesAttr().Get(), cp)

    AssertEqual(camera.GetFStopAttr().Get(), 0.0)
    camera.GetFStopAttr().Set(2.8)
    AssertClose(camera.GetFStopAttr().Get(), 2.8)

    AssertEqual(camera.GetFocusDistanceAttr().Get(), 0.0)
    camera.GetFocusDistanceAttr().Set(10.0)
    AssertEqual(camera.GetFocusDistanceAttr().Get(), 10.0)

def TestPoints():
    stage = Usd.Stage.CreateInMemory()

    # Points Tests

    schema = UsdGeom.Points.Define(stage, "/Points")
    AssertTrue(schema)
    
    # Test that id's roundtrip properly, for big numbers, and negative numbers
    ids = [8589934592, 1099511627776, 0, -42]
    schema.CreateIdsAttr(ids)
    resolvedIds = list(schema.GetIdsAttr().Get()) # convert VtArray to list
    AssertEqual(ids, resolvedIds)
 

def TestBug111239():
    # This bug broke the schema registry for prims whose typenames authored in
    # scene description were their canonical C++ typenames, rather than their
    # alias under UsdSchemaBase.  For example, if you had a prim with typename
    # 'UsdGeomSphere' instead of 'Sphere', we would not find builtins for the
    # prim with typename 'UsdGeomSphere'.
    s = Usd.Stage.CreateInMemory()
    sphere = s.DefinePrim('/sphere', typeName='Sphere')
    usdGeomSphere = s.DefinePrim('/usdGeomSphere', typeName='UsdGeomSphere')
    assert 'radius' in [a.GetName() for a in sphere.GetAttributes()]
    assert 'radius' in [a.GetName() for a in usdGeomSphere.GetAttributes()]

def TestComputeExtent():

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
    AssertTrue(extremeExtentRange.IsEmpty())

    # PointBased Test
    numDataSets = len(allPoints)
    for i in range(numDataSets):
        pointsData = allPoints[i]

        expectedExtent = pointBasedSolutions[i]
        actualExtent = UsdGeom.PointBased.ComputeExtent(pointsData)

        AssertClose(expectedExtent, actualExtent)

    # Points Test
    for i in range(numDataSets):
        pointsData = allPoints[i]
        widthsData = allWidths[i]

        expectedExtent = pointsSolutions[i]
        actualExtent = UsdGeom.Points.ComputeExtent(pointsData, widthsData)

        AssertClose(expectedExtent, actualExtent)

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

        AssertClose(expectedExtent, actualExtent)

def TestTypeUsage():
    # Perform Type-Ness Checking for ComputeExtent
    pointsAsList = [(0, 0, 0), (1, 1, 1), (2, 2, 2)]
    pointsAsVec3fArr = Vt.Vec3fArray(pointsAsList);

    comp = UsdGeom.PointBased.ComputeExtent
    AssertClose(comp(pointsAsList), comp(pointsAsVec3fArr))

def TestBug116593():
    from pxr import Gf

    s = Usd.Stage.CreateInMemory()
    prim = s.DefinePrim('/sphere', typeName='Sphere')

    # set with list of tuples
    vec = [(1,2,2),(12,3,3)]
    assert UsdGeom.ModelAPI(prim).SetExtentsHint(vec)
    assert UsdGeom.ModelAPI(prim).GetExtentsHint()[0] == Gf.Vec3f(1,2,2)
    assert UsdGeom.ModelAPI(prim).GetExtentsHint()[1] == Gf.Vec3f(12,3,3)

    # set with Gf vecs
    vec = [Gf.Vec3f(1,2,2), Gf.Vec3f(1,1,1)]
    assert UsdGeom.ModelAPI(prim).SetExtentsHint(vec)
    assert UsdGeom.ModelAPI(prim).GetExtentsHint()[0] == Gf.Vec3f(1,2,2)
    assert UsdGeom.ModelAPI(prim).GetExtentsHint()[1] == Gf.Vec3f(1,1,1)

def Main(argv):
    BasicTest()
    TestIsA()
    TestFallbacks()
    TestCamera()
    TestPoints()
    DefineSchema()
    BasicMetadataCases()
    TestBug111239()
    TestComputeExtent()
    TestTypeUsage()
    TestBug116593()

if __name__ == "__main__":
    SetAssertMode(MTR_EXIT_TEST)
    Main(sys.argv)
