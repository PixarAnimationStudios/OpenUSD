#!/pxrpythonsubst

from pxr import Sdf, Usd, UsdRi, UsdShade
from Mentor.Runtime import Assert,\
                           AssertEqual,\
                           AssertNotEqual,\
                           AssertClose,\
                           AssertTrue,\
                           AssertFalse,\
                           SetAssertMode,\
                           MTR_EXIT_TEST

def _TestSingleTargetRel(schema, getRelFn, createRelFn, getTargetFn, validTargetObjectPath):
    rel = getRelFn(schema)
    assert not rel
    rel = createRelFn(schema)
    assert rel

    # Add a valid target object
    rel.AddTarget(validTargetObjectPath)
    assert getTargetFn(schema)
    # Add one more target to the rel which should cause it to 
    # return nothing since it expects exactly one valid target.
    rel.AddTarget(schema.GetPrim().GetPath())
    assert not getTargetFn(schema)
    # Clear targets and add one again that is not a prim path, that should
    # also be an error condition.
    # XXX:Note here is where we'd like a validation scheme for USD.
    rel.ClearTargets(True)
    rel.AddTarget(schema.GetPrim().GetPath())
    assert not getTargetFn(schema)
    # Clean up
    rel.ClearTargets(True)
    assert not getTargetFn(schema)

def _TestMultiTargetRel(schema, getRelFn, createRelFn, getTargetFn, validTargetObjectPath):
    rel = getRelFn(schema)
    assert not rel
    rel = createRelFn(schema)
    assert rel

    rel.AddTarget(validTargetObjectPath)
    assert rel
    assert len(getTargetFn(schema)) == 1
    # Add an invalid target and make sure that we still get one valid object.
    rel.AddTarget(schema.GetPrim().GetPath())
    assert len(getTargetFn(schema)) == 1
    # Clean up
    rel.ClearTargets(True)


def BasicTest():
    l = Sdf.Layer.CreateAnonymous()
    stage = Usd.Stage.Open(l.identifier)

    world = stage.DefinePrim("/World", "Xform")
    assert world
    world.SetMetadata('kind', 'group')
    assert world.IsModel()
    assert world.IsGroup()

    group = stage.DefinePrim("/World/Group", "Xform")
    assert group
    group.SetMetadata('kind', 'group')
    assert group.IsModel()
    assert group.IsGroup()

    model = stage.DefinePrim("/World/Group/Model", "Xform")
    assert model
    model.SetMetadata('kind', 'char')
    assert model.IsModel()

    p = stage.DefinePrim("/World/Group/Model/Mesh", "Scope")
    assert p

    print ("Test look")
    look = UsdShade.Look.Define(stage, "/World/Group/Model/Look")
    assert look
    assert look.GetPrim()
    look.Bind(p)

    print ("Test shader")
    shader = UsdRi.RslShader.Define(stage, '/World/Group/Model/Shader')
    assert shader
    assert shader.GetPrim()
    assert not UsdRi.Statements.IsRiAttribute(shader.GetSloPathAttr())
    shader.GetSloPathAttr().Set('foo')

    print ("Test RiLookAPI")
    rilook = UsdRi.LookAPI(look)
    assert rilook 
    assert rilook.GetPrim()

    # Test surface rel
    _TestSingleTargetRel(rilook, 
        UsdRi.LookAPI.GetSurfaceRel, 
        UsdRi.LookAPI.CreateSurfaceRel,
        UsdRi.LookAPI.GetSurface,
        shader.GetPath())

    # Test displacement rel
    _TestSingleTargetRel(rilook, 
        UsdRi.LookAPI.GetDisplacementRel, 
        UsdRi.LookAPI.CreateDisplacementRel,
        UsdRi.LookAPI.GetDisplacement,
        shader.GetPath())

    # Test volume rel
    _TestSingleTargetRel(rilook, 
        UsdRi.LookAPI.GetVolumeRel, 
        UsdRi.LookAPI.CreateVolumeRel,
        UsdRi.LookAPI.GetVolume,
        shader.GetPath())

    # Test coshaders rel
    _TestMultiTargetRel(rilook,
        UsdRi.LookAPI.GetCoshadersRel,
        UsdRi.LookAPI.CreateCoshadersRel,
        UsdRi.LookAPI.GetCoshaders,
        shader.GetPath())

    print ("Test pattern")
    pattern = UsdRi.RisPattern.Define(stage, '/World/Group/Model/Pattern')
    assert pattern
    assert pattern.GetPrim()
    pattern.GetFilePathAttr().Set('foo')
    AssertEqual (pattern.GetFilePathAttr().Get(), 'foo')
    pattern.GetArgsPathAttr().Set('argspath')
    AssertEqual (pattern.GetArgsPathAttr().Get(), 'argspath')

    print ("Test oslPattern")
    oslPattern = UsdRi.RisOslPattern.Define(
        stage, '/World/Group/Model/OslPattern')
    assert oslPattern
    assert oslPattern.GetPrim()
    AssertEqual (oslPattern.GetFilePathAttr().Get(), 'PxrOSL')

    print ("Test bxdf")
    bxdf = UsdRi.RisBxdf.Define(stage, '/World/Group/Model/Bxdf')
    assert bxdf
    assert bxdf.GetPrim()
    bxdf.GetFilePathAttr().Set('foo')
    bxdf.GetArgsPathAttr().Set('argspath')

    print ("Test RIS Look")
    rislook = UsdRi.LookAPI(look.GetPrim())
    assert rislook 
    assert rislook.GetPrim()
    assert not rislook.GetBxdf()
    assert len(rislook.GetPatterns()) == 0

    # Test the bxdf relationship
    _TestSingleTargetRel(rislook,
        UsdRi.LookAPI.GetBxdfRel,
        UsdRi.LookAPI.CreateBxdfRel,
        UsdRi.LookAPI.GetBxdf,
        bxdf.GetPath())

    # Test the patterns relationship
    _TestMultiTargetRel(rislook,
        UsdRi.LookAPI.GetPatternsRel,
        UsdRi.LookAPI.CreatePatternsRel,
        UsdRi.LookAPI.GetPatterns,
        pattern.GetPath())

    print ("Test riStatements")
    riStatements = UsdRi.Statements(shader.GetPrim())
    assert riStatements
    assert riStatements.GetPrim()
    attr = riStatements.CreateRiAttribute("ModelName", "string").\
        Set('someModelName')
    assert attr
    props = riStatements.GetRiAttributes()
    assert props
    # this is so convoluted
    attr = riStatements.GetPrim().GetAttribute(props[0].GetName())
    assert attr
    AssertEqual(attr.GetName(), 'ri:attributes:user:ModelName')
    AssertEqual(attr.Get(), 'someModelName')
    AssertEqual(UsdRi.Statements.GetRiAttributeName(attr), 'ModelName')
    AssertEqual(UsdRi.Statements.GetRiAttributeNameSpace(attr), 'user')
    assert UsdRi.Statements.IsRiAttribute(attr)

    AssertEqual(UsdRi.Statements.MakeRiAttributePropertyName('myattr'),
                'ri:attributes:user:myattr')
    AssertEqual(UsdRi.Statements.MakeRiAttributePropertyName('dice:myattr'),
                'ri:attributes:dice:myattr')
    AssertEqual(UsdRi.Statements.MakeRiAttributePropertyName('dice.myattr'),
                'ri:attributes:dice:myattr')
    AssertEqual(UsdRi.Statements.MakeRiAttributePropertyName('dice_myattr'),
                'ri:attributes:dice:myattr')
    # period is stronger separator than underscore, when both are present
    AssertEqual(UsdRi.Statements.MakeRiAttributePropertyName('dice_my.attr'),
                'ri:attributes:dice_my:attr')
    # multiple tokens concatted with underscores
    AssertEqual(UsdRi.Statements.MakeRiAttributePropertyName('dice:my1:long:attr'),
                'ri:attributes:dice:my1_long_attr')
    AssertEqual(UsdRi.Statements.MakeRiAttributePropertyName('dice.my2.long.attr'),
                'ri:attributes:dice:my2_long_attr')
    AssertEqual(UsdRi.Statements.MakeRiAttributePropertyName('dice_my3_long_attr'),
                'ri:attributes:dice:my3_long_attr')

    AssertEqual(riStatements.GetCoordinateSystem(), '')
    AssertEqual(UsdRi.Statements(model).GetModelCoordinateSystems(), [])
    AssertEqual(UsdRi.Statements(model).GetModelScopedCoordinateSystems(), [])
    riStatements.SetCoordinateSystem('LEyeSpace')
    AssertEqual(riStatements.GetCoordinateSystem(), 'LEyeSpace')
    AssertEqual(UsdRi.Statements(model).GetModelCoordinateSystems(),
                [Sdf.Path('/World/Group/Model/Shader')])
    riStatements.SetScopedCoordinateSystem('ScopedLEyeSpace')
    AssertEqual(riStatements.GetScopedCoordinateSystem(), 'ScopedLEyeSpace')
    AssertEqual(UsdRi.Statements(model).GetModelScopedCoordinateSystems(),
                [Sdf.Path('/World/Group/Model/Shader')])
    AssertEqual(UsdRi.Statements(group).GetModelCoordinateSystems(), [])
    AssertEqual(UsdRi.Statements(group).GetModelScopedCoordinateSystems(), [])
    AssertEqual(UsdRi.Statements(world).GetModelCoordinateSystems(), [])
    AssertEqual(UsdRi.Statements(world).GetModelScopedCoordinateSystems(), [])

    AssertFalse(riStatements.GetFocusRegionAttr().IsValid())
    Assert(riStatements.CreateFocusRegionAttr() is not None)
    Assert(riStatements.GetFocusRegionAttr() is not None)
    AssertTrue(riStatements.GetFocusRegionAttr().IsValid())
    AssertEqual(riStatements.GetFocusRegionAttr().Get(), None)
    riStatements.CreateFocusRegionAttr(9.0, True)
    AssertEqual(riStatements.GetFocusRegionAttr().Get(), 9.0)
    
def TestMetadata():
    stage = Usd.Stage.CreateInMemory()
    osl = UsdRi.RisOslPattern.Define(stage, "/osl")
    AssertTrue( osl.GetFilePathAttr().IsHidden() )

def Main(argv):
    BasicTest()
    TestMetadata()
    
    print ("Tests successful")



if __name__ == "__main__":
    SetAssertMode(MTR_EXIT_TEST)

    import sys
    Main(sys.argv)
