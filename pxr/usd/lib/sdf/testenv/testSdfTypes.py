#!/pxrpythonsubst

from Mentor.Runtime import (
    SetAssertMode, MTR_EXIT_TEST, Assert, AssertEqual, RequiredException)

from pxr import Sdf, Vt, Gf

def TestTypeValidity():
    class Unknown(object):
        pass

    Assert(not Sdf.ValueHasValidType(None))
    Assert(not Sdf.ValueHasValidType(Unknown()))

def TestRoundTrippingValue(typeName, value):
    # Create a layer with custom attribute of this type, set its value,
    # export it to a string, reimport it from a string and assert that it
    # round-tripped the value correctly.
    layer = Sdf.Layer.CreateAnonymous()
    prim = Sdf.CreatePrimInLayer(layer, '/test')
    attrSpec = Sdf.AttributeSpec(prim, 'testAttr', 
                                 Sdf.ValueTypeNames.Find(typeName))
    attrSpec.default = value
    layerAsString = layer.ExportToString()
    # Now create a new layer and import it.
    layer = Sdf.Layer.CreateAnonymous()
    layer.ImportFromString(layerAsString)
    assert layer.GetAttributeAtPath('/test.testAttr').default == value

def TestValueValidity():
    validValues = [
        ("hello",'string'),
        (True,'bool'),
        (1,'uchar'),
        (1,'int'),
        (1,'uint'),
        (1,'int64'),
        (1,'uint64'),
        (1.0,'half'),
        (1.0,'float'),
        (1.0,'double'),
        (Gf.Vec2d(1,2),'double2'),
        (Gf.Vec2f(1,2),'float2'),
        (Gf.Vec2h(1,2),'half2'),
        (Gf.Vec2i(1,2),'int2'),
        (Gf.Vec3d(1,2,3),'double3'),
        (Gf.Vec3f(1,2,3),'float3'),
        (Gf.Vec3h(1,2,3),'half3'),
        (Gf.Vec3i(1,2,3),'int3'),
        (Gf.Vec4d(1,2,3,4),'double4'),
        (Gf.Vec4f(1,2,3,4),'float4'),
        (Gf.Vec4h(1,2,3,4),'half4'),
        (Gf.Vec4i(1,2,3,4),'int4'),
        (Gf.Matrix4d(3),'matrix4d'),
        (Gf.Quatd(1), 'quatd'),
        (Gf.Quatf(2), 'quatf'),
        (Gf.Quath(2), 'quath'),
        (Vt.StringArray(),'string[]'),
        (Vt.Vec2dArray(),'double2[]'),
        (Vt.Vec2fArray(),'float2[]'),
        (Vt.Vec2hArray(),'half2[]'),
        (Vt.Vec2iArray(),'int2[]')
    ]

    for value, typeName in validValues:
        Assert(Sdf.ValueHasValidType(value))

        # Special case -- python floats will always report 'double', and
        # integers will always report 'int' for the values we're testing here.
        if typeName in ['half', 'float']:
            AssertEqual(Sdf.GetValueTypeNameForValue(value), 'double')
        elif typeName in ['uchar', 'uint', 'int64', 'uint64']:
            AssertEqual(Sdf.GetValueTypeNameForValue(value), 'int') 
        else:
            AssertEqual(Sdf.GetValueTypeNameForValue(value), typeName)

        TestRoundTrippingValue(typeName, value)

# Test types like Color3f which correspond to a basic type.
#
# For example, GetValueTypeNameForValue() called with a Vec3d() will 
# return the most general type, double3, even if the value came
# from a color3d attribute, so we test to see if the values are
# properly aliasing.
def TestAliasedTypes():
    # A list of value, type and the corresponding basic type. 
    validAliasValues = [
        # Color types
        (Gf.Vec3d(1,2,3),'color3d', 'double3'),
        (Gf.Vec3f(1,2,3),'color3f', 'float3'),
        (Gf.Vec3h(1,2,3),'color3h', 'half3'),
        (Gf.Vec4d(1,2,3,4),'color4d', 'double4'),
        (Gf.Vec4f(1,2,3,4),'color4f', 'float4'),
        (Gf.Vec4h(1,2,3,4),'color4h', 'half4'),
        (Vt.Vec3dArray(), 'color3d[]', 'double3[]'), 
        (Vt.Vec3fArray(), 'color3f[]', 'float3[]'), 
        (Vt.Vec3hArray(), 'color3h[]', 'half3[]'), 
        (Vt.Vec4dArray(), 'color4d[]', 'double4[]'), 
        (Vt.Vec4fArray(), 'color4f[]', 'float4[]'), 
        (Vt.Vec4hArray(), 'color4h[]', 'half4[]'), 
    
        # Point types
        (Gf.Vec3d(1,2,3),'point3d', 'double3'),
        (Gf.Vec3f(1,2,3),'point3f', 'float3'),
        (Gf.Vec3h(1,2,3),'point3h', 'half3'),
        (Vt.Vec3dArray(), 'point3d[]', 'double3[]'), 
        (Vt.Vec3fArray(), 'point3f[]', 'float3[]'), 
        (Vt.Vec3hArray(), 'point3h[]', 'half3[]'), 
   
        # Normal types
        (Gf.Vec3d(1,2,3),'normal3d', 'double3'),
        (Gf.Vec3f(1,2,3),'normal3f', 'float3'),
        (Gf.Vec3h(1,2,3),'normal3h', 'half3'),
        (Vt.Vec3dArray(), 'normal3d[]', 'double3[]'), 
        (Vt.Vec3fArray(), 'normal3f[]', 'float3[]'), 
        (Vt.Vec3hArray(), 'normal3h[]', 'half3[]'), 

        # Vector types
        (Gf.Vec3d(1,2,3),'vector3d', 'double3'),
        (Gf.Vec3f(1,2,3),'vector3f', 'float3'),
        (Gf.Vec3h(1,2,3),'vector3h', 'half3'),
        (Vt.Vec3dArray(), 'vector3d[]', 'double3[]'), 
        (Vt.Vec3fArray(), 'vector3f[]', 'float3[]'), 
        (Vt.Vec3hArray(), 'vector3h[]', 'half3[]'), 
   
        # Frame type
        (Gf.Matrix4d(3), 'frame4d', 'matrix4d')
    ]

    for value, typeName, baseTypeName in validAliasValues:
        Assert(Sdf.ValueHasValidType(value))
        AssertEqual(Sdf.GetValueTypeNameForValue(value), baseTypeName)
        TestRoundTrippingValue(typeName, value)

########################################################################
# Test that we will not parse files that have out-of-range int values.
def TestOutOfRangeIntSettingAndParsing():
    types = [dict(name='uchar', min=0, max=255),
             dict(name='int', min=-2**31, max=2**31-1),
             dict(name='uint', min=0, max=2**32-1),
             dict(name='int64', min=-2**63, max=2**63-1),
             dict(name='uint64', min=0, max=2**64-1)]

    layer = Sdf.Layer.CreateAnonymous()
    p = Sdf.CreatePrimInLayer(layer, '/p')
    a = Sdf.AttributeSpec(p, 'attr', Sdf.ValueTypeNames.Int)
    for t in types:
        # XXX: This test uses the SetInfo() API in a really backdoor way.  We
        # use it to write SdfUnregisteredValue objects into layers in order to
        # get essentially arbitrary text output.  We use that to write numerical
        # value strings that are out of range for given attribute types.  An
        # alternative approach would be to provide a template string that we
        # attempt to parse, but that assumes the current text file format.
        
        # Overwrite scene description and attempt to write/read.
        a.SetInfo('typeName', t['name'])

        # Min and max values should come through okay.
        a.SetInfo('default', Sdf.UnregisteredValue(str(t['min'])))
        layer.ImportFromString(layer.ExportToString())
        a.SetInfo('default', Sdf.UnregisteredValue(str(t['max'])))
        layer.ImportFromString(layer.ExportToString())

        # One more than the max and one less than the min should fail.
        a.SetInfo('default', Sdf.UnregisteredValue(str(t['min'] - 1)))
        with RequiredException(RuntimeError):
            layer.ImportFromString(layer.ExportToString())

        a.SetInfo('default', Sdf.UnregisteredValue(str(t['max'] + 1)))
        with RequiredException(RuntimeError):
            layer.ImportFromString(layer.ExportToString())

        # Try setting attribute values that are out of range.
        l = Sdf.Layer.CreateAnonymous()
        q = Sdf.CreatePrimInLayer(l, '/q')
        b = Sdf.AttributeSpec(q, 'attr', Sdf.ValueTypeNames.Find(t['name']))
        b.default = t['min']
        AssertEqual(b.default, t['min'])
        b.default = t['max']
        AssertEqual(b.default, t['max'])
        # Out of range should fail.
        with RequiredException((RuntimeError, OverflowError)):
            b.default = t['min'] - 1
        with RequiredException((RuntimeError, OverflowError)):
            b.default = t['max'] + 1
            
########################################################################
# Test that new value type names are backwards-compatible with
# old type names.
def TestValueTypeNameBackwardsCompatibility():
    layer = Sdf.Layer.CreateAnonymous()
    layer.ImportFromString(\
"""#sdf 1.4.32

def "OldAttrTest"
{
    Vec2i a1
    Vec2h a2
    Vec2f a3
    Vec2d a4
    Vec3i a5
    Vec3h a6
    Vec3f a7
    Vec3d a8
    Vec4i a9
    Vec4h a10
    Vec4f a11
    Vec4d a12
    Point a13
    PointFloat a14
    Normal a15
    NormalFloat a16
    Vector a17
    VectorFloat a18
    Color a19
    ColorFloat a20
    Quath a21
    Quatf a22
    Quatd a23
    Matrix2d a24
    Matrix3d a25
    Matrix4d a26
    Frame a27
} 
""")

    def _TestValueTypeName(attrPath, expectedValueTypeName):
        attr = layer.GetAttributeAtPath(attrPath)
        assert attr.typeName == expectedValueTypeName, \
            "Type name %s did not match %s" % \
            (str(attr.typeName), str(expectedValueTypeName))
        assert hash(attr.typeName) == hash(expectedValueTypeName), \
            "Hash for type name %s did not match %s" % \
            (str(attr.typeName), str(expectedValueTypeName))

    _TestValueTypeName("/OldAttrTest.a1", Sdf.ValueTypeNames.Int2)
    _TestValueTypeName("/OldAttrTest.a2", Sdf.ValueTypeNames.Half2)
    _TestValueTypeName("/OldAttrTest.a3", Sdf.ValueTypeNames.Float2)
    _TestValueTypeName("/OldAttrTest.a4", Sdf.ValueTypeNames.Double2)
    _TestValueTypeName("/OldAttrTest.a5", Sdf.ValueTypeNames.Int3)
    _TestValueTypeName("/OldAttrTest.a6", Sdf.ValueTypeNames.Half3)
    _TestValueTypeName("/OldAttrTest.a7", Sdf.ValueTypeNames.Float3)
    _TestValueTypeName("/OldAttrTest.a8", Sdf.ValueTypeNames.Double3)
    _TestValueTypeName("/OldAttrTest.a9", Sdf.ValueTypeNames.Int4)
    _TestValueTypeName("/OldAttrTest.a10", Sdf.ValueTypeNames.Half4)
    _TestValueTypeName("/OldAttrTest.a11", Sdf.ValueTypeNames.Float4)
    _TestValueTypeName("/OldAttrTest.a12", Sdf.ValueTypeNames.Double4)
    _TestValueTypeName("/OldAttrTest.a13", Sdf.ValueTypeNames.Point3d)
    _TestValueTypeName("/OldAttrTest.a14", Sdf.ValueTypeNames.Point3f)
    _TestValueTypeName("/OldAttrTest.a15", Sdf.ValueTypeNames.Normal3d)
    _TestValueTypeName("/OldAttrTest.a16", Sdf.ValueTypeNames.Normal3f)
    _TestValueTypeName("/OldAttrTest.a17", Sdf.ValueTypeNames.Vector3d)
    _TestValueTypeName("/OldAttrTest.a18", Sdf.ValueTypeNames.Vector3f)
    _TestValueTypeName("/OldAttrTest.a19", Sdf.ValueTypeNames.Color3d)
    _TestValueTypeName("/OldAttrTest.a20", Sdf.ValueTypeNames.Color3f)
    _TestValueTypeName("/OldAttrTest.a21", Sdf.ValueTypeNames.Quath)
    _TestValueTypeName("/OldAttrTest.a22", Sdf.ValueTypeNames.Quatf)
    _TestValueTypeName("/OldAttrTest.a23", Sdf.ValueTypeNames.Quatd)
    _TestValueTypeName("/OldAttrTest.a24", Sdf.ValueTypeNames.Matrix2d)
    _TestValueTypeName("/OldAttrTest.a25", Sdf.ValueTypeNames.Matrix3d)
    _TestValueTypeName("/OldAttrTest.a26", Sdf.ValueTypeNames.Matrix4d)
    _TestValueTypeName("/OldAttrTest.a27", Sdf.ValueTypeNames.Frame4d)

TestTypeValidity()
TestValueValidity()
TestAliasedTypes()
TestOutOfRangeIntSettingAndParsing()
TestValueTypeNameBackwardsCompatibility()

print 'OK'        
        
        
        
