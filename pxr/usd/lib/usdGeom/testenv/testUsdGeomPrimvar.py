#!/pxrpythonsubst

import sys, os
from pxr import Usd, UsdGeom, Sdf, Tf, Vt

import Mentor.Runtime
from Mentor.Runtime import (AssertEqual, AssertTrue, AssertFalse, AssertClose,
                            AssertNotEqual, ExpectedErrors, RequiredException)

def TestPrimvarAPI():
    # We'll put all our Primvar on a single mesh gprim
    stage = Usd.Stage.CreateInMemory('myTest.usda')
    gp = UsdGeom.Mesh.Define(stage, '/myMesh')

    nPasses = 3

    # Add three Primvars
    u1 = gp.CreatePrimvar('u_1', Sdf.ValueTypeNames.FloatArray)
    # Make sure it's OK to manually specify the classifier namespace
    v1 = gp.CreatePrimvar('primvars:v_1', Sdf.ValueTypeNames.FloatArray)
    _3dpmats = gp.CreatePrimvar('projMats', Sdf.ValueTypeNames.Matrix4dArray,
                                "constant", nPasses)
    
    # ensure we can't create a primvar that contains namespaces.
    with RequiredException(Tf.ErrorException):
        gp.CreatePrimvar('no:can:do', Sdf.ValueTypeNames.FloatArray)
    
    AssertEqual(len( gp.GetAuthoredPrimvars() ), 3)
    AssertEqual(len( gp.GetPrimvars() ), 5)
    
    # Now add some random properties, and reverify
    p = gp.GetPrim()
    p.CreateRelationship("myBinding")
    p.CreateAttribute("myColor", Sdf.ValueTypeNames.Color3f)
    p.CreateAttribute("primvars:my:overly:namespaced:Color",
                      Sdf.ValueTypeNames.Color3f)

    datas = gp.GetAuthoredPrimvars()
    IsPrimvar = UsdGeom.Primvar.IsPrimvar

    AssertEqual(len(datas), 3)
    AssertTrue( IsPrimvar(datas[0]) )
    AssertTrue( IsPrimvar(datas[1]) )
    # For variety, test the explicit Attribute extractor
    AssertTrue( IsPrimvar(datas[2].GetAttr()) )
    AssertFalse( IsPrimvar(p.GetAttribute("myColor")) )
    # Here we're testing that the speculative constructor fails properly
    AssertFalse( IsPrimvar(UsdGeom.Primvar(p.GetAttribute("myColor"))) )
    # And here that the speculative constructor succeeds properly
    AssertTrue( IsPrimvar(UsdGeom.Primvar(p.GetAttribute(v1.GetName()))) )


    # Some of the same tests, exercising the bool-type operator
    # for UsdGeomPrimvar; Primvar provides the easiest way to get INvalid attrs!
    AssertTrue( datas[0] )
    AssertTrue( datas[1] )
    AssertTrue( datas[2] )
    AssertFalse( UsdGeom.Primvar(p.GetAttribute("myColor")) )
    AssertFalse( UsdGeom.Primvar(p.GetAttribute("myBinding")) )
    AssertTrue( UsdGeom.Primvar(p.GetAttribute(v1.GetName())) )
    
    # Same classification test through GprimSchema API
    AssertTrue( gp.HasPrimvar('u_1') )
    AssertTrue( gp.HasPrimvar('v_1') )
    AssertTrue( gp.HasPrimvar('projMats') )
    AssertFalse( gp.HasPrimvar('myColor') )
    AssertFalse( gp.HasPrimvar('myBinding') )

    # Test that the gpv's returned by GetPrimvars are REALLY valid,
    # and that the UsdAttribute metadata wrappers work
    AssertEqual( datas[0].GetTypeName(), Sdf.ValueTypeNames.Matrix4dArray )
    AssertEqual( datas[1].GetTypeName(), Sdf.ValueTypeNames.FloatArray )
    AssertEqual( datas[2].GetBaseName(), "v_1" )
    
    # Now we'll add some extra configuration and verify that the 
    # interrogative API works properly
    AssertEqual( u1.GetInterpolation(), UsdGeom.Tokens.constant )  # fallback
    AssertFalse( u1.HasAuthoredInterpolation() )
    AssertFalse( u1.HasAuthoredElementSize() )
    AssertTrue( u1.SetInterpolation(UsdGeom.Tokens.vertex) )
    AssertTrue( u1.HasAuthoredInterpolation() )
    AssertEqual( u1.GetInterpolation(), UsdGeom.Tokens.vertex )

    AssertFalse( v1.HasAuthoredInterpolation() )
    AssertFalse( v1.HasAuthoredElementSize() )
    AssertTrue( v1.SetInterpolation(UsdGeom.Tokens.uniform) )
    AssertTrue( v1.SetInterpolation(UsdGeom.Tokens.varying) )
    AssertTrue( v1.SetInterpolation(UsdGeom.Tokens.constant) )
    AssertTrue( v1.SetInterpolation(UsdGeom.Tokens.faceVarying) )

    with RequiredException(Tf.ErrorException):
        v1.SetInterpolation("frobosity")
    # Should be the last good value set
    AssertEqual( v1.GetInterpolation(), "faceVarying" )

    AssertTrue( _3dpmats.HasAuthoredInterpolation() )
    AssertTrue( _3dpmats.HasAuthoredElementSize() )
    with RequiredException(Tf.ErrorException):
        _3dpmats.SetElementSize(0)
    # Failure to set shouldn't change the state...
    AssertTrue( _3dpmats.HasAuthoredElementSize() )
    AssertTrue( _3dpmats.SetElementSize(nPasses) )
    AssertTrue( _3dpmats.HasAuthoredElementSize() )

    # Make sure value Get/Set work
    AssertEqual( u1.Get(), None )

    AssertFalse(u1.IsIndexed())
    AssertEqual(u1.ComputeFlattened(), None)

    uVal = Vt.FloatArray([1.1,2.1,3.1])
    AssertTrue(u1.Set(uVal))
    AssertEqual(u1.Get(), uVal)

    # Make sure indexed primvars work
    AssertFalse(u1.IsIndexed())
    indices = Vt.IntArray([0, 1, 2, 2, 1, 0])
    AssertTrue(u1.SetIndices(indices))
    AssertTrue(u1.IsIndexed())

    AssertEqual(u1.GetIndices(), indices)
    AssertClose(u1.ComputeFlattened(), 
                [1.1, 2.1, 3.1, 3.1, 2.1, 1.1])
    AssertNotEqual(u1.ComputeFlattened(), u1.Get())

    AssertEqual(u1.GetUnauthoredValuesIndex(), -1)
    AssertTrue(u1.SetUnauthoredValuesIndex(2))
    AssertEqual(u1.GetUnauthoredValuesIndex(), 2)

    indicesAt1 = Vt.IntArray([1,2,0])
    indicesAt2 = Vt.IntArray([])

    AssertTrue(u1.SetIndices(indicesAt1, 1.0))
    AssertEqual(u1.GetIndices(1.0), indicesAt1)

    AssertTrue(u1.SetIndices(indicesAt2, 2.0))
    AssertEqual(u1.GetIndices(2.0), indicesAt2)

    AssertClose(u1.ComputeFlattened(1.0), 
                [2.1, 3.1, 1.1])
    AssertNotEqual(u1.ComputeFlattened(1.0), u1.Get(1.0))

    AssertTrue(len(u1.ComputeFlattened(2.0)) == 0)
    AssertNotEqual(u1.ComputeFlattened(2.0), u1.Get(2.0))

    # Finally, ensure the values returned by GetDeclarationInfo
    # (on new Primvar objects, to test the GprimSchema API)
    # is identical to the individual queries, and matches what we set above
    nu1 = gp.GetPrimvar("u_1")
    (name, typeName, interpolation, elementSize) = nu1.GetDeclarationInfo()
    AssertEqual(name,          u1.GetBaseName())
    AssertEqual(typeName,      u1.GetTypeName())
    AssertEqual(interpolation, u1.GetInterpolation())
    AssertEqual(elementSize,   u1.GetElementSize())
    
    AssertEqual(name,          "u_1")
    AssertEqual(typeName,      Sdf.ValueTypeNames.FloatArray)
    AssertEqual(interpolation, UsdGeom.Tokens.vertex)
    AssertEqual(elementSize,   1)
    
    nv1 = gp.GetPrimvar("v_1")
    (name, typeName, interpolation, elementSize) = nv1.GetDeclarationInfo()
    AssertEqual(name,          v1.GetBaseName())
    AssertEqual(typeName,      v1.GetTypeName())
    AssertEqual(interpolation, v1.GetInterpolation())
    AssertEqual(elementSize,   v1.GetElementSize())
    
    AssertEqual(name,          "v_1")
    AssertEqual(typeName,      Sdf.ValueTypeNames.FloatArray)
    AssertEqual(interpolation, UsdGeom.Tokens.faceVarying)
    AssertEqual(elementSize,   1)

    nmats = gp.GetPrimvar('projMats')
    (name, typeName, interpolation, elementSize) = nmats.GetDeclarationInfo()
    AssertEqual(name,          _3dpmats.GetBaseName())
    AssertEqual(typeName,      _3dpmats.GetTypeName())
    AssertEqual(interpolation, _3dpmats.GetInterpolation())
    AssertEqual(elementSize,   _3dpmats.GetElementSize())
    
    AssertEqual(name,          'projMats')
    AssertEqual(typeName,      Sdf.ValueTypeNames.Matrix4dArray)
    AssertEqual(interpolation, UsdGeom.Tokens.constant)
    AssertEqual(elementSize,   nPasses)

    # Id primvar
    notId = gp.CreatePrimvar('notId', Sdf.ValueTypeNames.FloatArray)
    AssertFalse(notId.IsIdTarget())
    with RequiredException(Tf.ErrorException):
        notId.SetIdTarget(gp.GetPath())

    handleid = gp.CreatePrimvar('handleid', Sdf.ValueTypeNames.String)

    # make sure we can still just set a string
    v = "handleid_value"
    AssertTrue(handleid.Set(v))
    AssertEqual(handleid.Get(), v)
    AssertEqual(handleid.ComputeFlattened(), v)

    numPrimvars = len(gp.GetPrimvars())
    
    # This check below ensures that the "indices" attributes belonging to 
    # indexed primvars aren't considered to be primvars themselves.
    AssertEqual(numPrimvars, 7)

    AssertTrue(handleid.SetIdTarget(gp.GetPath()))
    # make sure we didn't increase the number of primvars (also that
    # GetPrimvars doesn't break when we have relationships)
    AssertEqual(len(gp.GetPrimvars()), numPrimvars)
    AssertEqual(handleid.Get(), gp.GetPath())

    stringPath = '/my/string/path'
    AssertTrue(handleid.SetIdTarget(stringPath))
    AssertEqual(handleid.Get(), Sdf.Path(stringPath))

    p = Sdf.Path('/does/not/exist')
    AssertTrue(handleid.SetIdTarget(p))
    AssertEqual(handleid.Get(), p)

    handleid_array = gp.CreatePrimvar('handleid_array', Sdf.ValueTypeNames.StringArray)
    AssertTrue(handleid_array.SetIdTarget(gp.GetPath()))

def TestBug124579():
    from pxr import Usd
    from pxr import UsdGeom
    from pxr import Vt
    
    stage = Usd.Stage.CreateInMemory()
    
    gprim = UsdGeom.Mesh.Define(stage, '/myMesh')
    
    primvar = gprim.CreatePrimvar('myStringArray', Sdf.ValueTypeNames.StringArray)
    primvar.SetInterpolation(UsdGeom.Tokens.constant)
    
    value = ['one', 'two', 'three']
    primvar.Set(value)
    
    # Fetching the value back out requires our special python type-magic
    # that only triggers when we fetch by VtValue...
    # that is what we are testing here
    AssertEqual(primvar.Get(), Vt.StringArray(len(value), value))

if __name__ == "__main__":
    TestPrimvarAPI()
    TestBug124579()
