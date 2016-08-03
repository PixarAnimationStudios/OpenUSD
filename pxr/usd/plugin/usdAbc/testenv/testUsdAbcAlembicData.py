#!/pxrpythonsubst

import sys, os, tempfile
from pxr import Sdf, Usd, Vt, Gf, Tf, UsdAbc

import Mentor.Runtime


def Main():
    stage = Usd.Stage.Open('/usr/anim/menv30/testing/usd/MUDormRoomMikeRandy_set/modsets/MUDormRoomMikeRandy_set/usd/MUDormRoomMikeRandy_set.abc')
    assert stage

    prim = stage.GetPrimAtPath('/MUDormRoomMikeRandy_set/MUDormRoomMikeRandy_set1')
    assert prim
    pts = prim.GetAttribute('points')
    assert pts
    assert pts.IsDefined()



def WriteTest():
    # Create a usda and abc temporary files.
    # NOTE: This files will automatically be deleted when the test quits,
    # if you want to keep them around for debugging, pass in delete=False
    tempAbcFile = tempfile.NamedTemporaryFile(suffix='.abc')
    tempUsdFile = tempfile.NamedTemporaryFile(suffix='.usda')

    # Create a USD file that we'll save out as .abc
    stage = Usd.Stage.CreateNew(tempUsdFile.name)
    stage.OverridePrim('/Prim')
    stage.GetRootLayer().Save()

    # Write out the USD file as .abc
    UsdAbc._WriteAlembic(tempUsdFile.name, tempAbcFile.name)

    # Read it back in and expect to read back a prim.
    roundTrippedStage = Usd.Stage.Open(tempAbcFile.name)
    prim = roundTrippedStage.GetPrimAtPath('/Prim')
    assert prim

    # Verify that timeCodesPerSecond and framesPerSecond values are preserved 
    # when round-tripping.
    assert roundTrippedStage.GetRootLayer().HasTimeCodesPerSecond()
    Mentor.Runtime.AssertEqual(roundTrippedStage.GetTimeCodesPerSecond(), 
                               stage.GetTimeCodesPerSecond())

    assert roundTrippedStage.GetRootLayer().HasFramesPerSecond()
    Mentor.Runtime.AssertEqual(roundTrippedStage.GetFramesPerSecond(), 
                               stage.GetTimeCodesPerSecond())


def TypeTest():
    # Create a usda and abc temporary files.
    # NOTE: This files will automatically be deleted when the test quits,
    # if you want to keep them around for debugging, pass in delete=False
    tempAbcFile = tempfile.NamedTemporaryFile(suffix='.abc')
    tempUsdFile = tempfile.NamedTemporaryFile(suffix='.usda')

    stage = Usd.Stage.CreateNew(tempUsdFile.name)
    prim = stage.OverridePrim('/Prim')

    # Note these test cases come from testSdTypes.py
    usdValuesToTest = [
        ("hello", 'string', 'myString'),
        (True,'bool', 'myBool'),
        (1,'uchar', 'myUChar'),
        (1,'int', 'myInt'),
        (1,'uint', 'myUInt'),
        (1,'int64', 'myInt64'), 
        (1,'uint64', 'myUInt64'), 
        (1.0,'half', 'myHalf'), 
        (1.0,'float', 'myFloat'), 
        (1.0,'double', 'myDouble'), 
        (Gf.Vec2d(1,2),'double2', 'myVec2d'),
        (Gf.Vec2f(1,2),'float2', 'myVec2f'), 
        (Gf.Vec2h(1,2),'half2', 'myVec2h'), 
        (Gf.Vec2i(1,2),'int2', 'myVec2i'), 
        (Gf.Vec3d(1,2,3),'double3', 'myVec3d'), 
        (Gf.Vec3f(1,2,3),'float3', 'myVec3f'), 
        (Gf.Vec3h(1,2,3),'half3', 'myVec3h'), 
        (Gf.Vec3i(1,2,3),'int3', 'myVec3i'), 
        (Gf.Vec4d(1,2,3,4),'double4', 'myVec4d'), 
        (Gf.Vec4f(1,2,3,4),'float4', 'myVec4f'), 
        (Gf.Vec4h(1,2,3,4),'half4', 'myVec4h'), 
        (Gf.Vec4i(1,2,3,4),'int4', 'myVec4i'), 
        (Gf.Matrix4d(3),'matrix4d', 'myMatrix4d'), 
        (Gf.Quatf(1.0, [2.0, 3.0, 4.0]), 'quatf', 'myQuatf'),
        (Gf.Quatd(1.0, [2.0, 3.0, 4.0]), 'quatd', 'myQuatd'),
        (Vt.StringArray(),'string[]', 'myStringArray'), 
        (Vt.Vec2dArray(),'double2[]', 'myVec2dArray'), 
        (Vt.Vec2fArray(),'float2[]', 'myVec2fArray'), 
        (Vt.Vec2hArray(),'half2[]', 'myVec2hArray'), 
        (Vt.Vec2iArray(),'int2[]', 'myVec2iArray'), 
    ]

    for value, typeName, attrName in usdValuesToTest:
        prim.CreateAttribute(attrName, Sdf.ValueTypeNames.Find(typeName))
        prim.GetAttribute(attrName).Set(value)

    stage.GetRootLayer().Save()

    # Write out the USD file as .abc
    UsdAbc._WriteAlembic(tempUsdFile.name, tempAbcFile.name)

    # Read it back in and expect the same attributes and values
    stage = Usd.Stage.Open(tempAbcFile.name)
    prim = stage.GetPrimAtPath('/Prim')
    assert prim

    for value, typeName, attrName in usdValuesToTest:
        attr = prim.GetAttribute(attrName)
        assert attr
        assert attr.GetTypeName() == typeName
        assert attr.Get() == value
            

if __name__ == "__main__":
    Mentor.Runtime.SetAssertMode(Mentor.Runtime.MTR_EXIT_TEST)
    Main()
    WriteTest()
    TypeTest()
