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

import sys, os, unittest
from pxr import Gf, Usd, UsdGeom, Sdf, Tf, Vt

class TestUsdGeomPrimvarAPI(unittest.TestCase):
    def test_PrimvarAPI(self):
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
        with self.assertRaises(Tf.ErrorException):
            gp.CreatePrimvar('no:can:do', Sdf.ValueTypeNames.FloatArray)
        
        self.assertEqual(len( gp.GetAuthoredPrimvars() ), 3)
        self.assertEqual(len( gp.GetPrimvars() ), 5)
        
        # Now add some random properties, and reverify
        p = gp.GetPrim()
        p.CreateRelationship("myBinding")
        p.CreateAttribute("myColor", Sdf.ValueTypeNames.Color3f)
        p.CreateAttribute("primvars:my:overly:namespaced:Color",
                          Sdf.ValueTypeNames.Color3f)

        datas = gp.GetAuthoredPrimvars()
        IsPrimvar = UsdGeom.Primvar.IsPrimvar

        self.assertEqual(len(datas), 3)
        self.assertTrue( IsPrimvar(datas[0]) )
        self.assertTrue( IsPrimvar(datas[1]) )
        # For variety, test the explicit Attribute extractor
        self.assertTrue( IsPrimvar(datas[2].GetAttr()) )
        self.assertFalse( IsPrimvar(p.GetAttribute("myColor")) )
        # Here we're testing that the speculative constructor fails properly
        self.assertFalse( IsPrimvar(UsdGeom.Primvar(p.GetAttribute("myColor"))) )
        # And here that the speculative constructor succeeds properly
        self.assertTrue( IsPrimvar(UsdGeom.Primvar(p.GetAttribute(v1.GetName()))) )


        # Some of the same tests, exercising the bool-type operator
        # for UsdGeomPrimvar; Primvar provides the easiest way to get INvalid attrs!
        self.assertTrue( datas[0] )
        self.assertTrue( datas[1] )
        self.assertTrue( datas[2] )
        self.assertFalse( UsdGeom.Primvar(p.GetAttribute("myColor")) )
        self.assertFalse( UsdGeom.Primvar(p.GetAttribute("myBinding")) )
        self.assertTrue( UsdGeom.Primvar(p.GetAttribute(v1.GetName())) )
        
        # Same classification test through GprimSchema API
        self.assertTrue( gp.HasPrimvar('u_1') )
        self.assertTrue( gp.HasPrimvar('v_1') )
        self.assertTrue( gp.HasPrimvar('projMats') )
        self.assertFalse( gp.HasPrimvar('myColor') )
        self.assertFalse( gp.HasPrimvar('myBinding') )

        # Test that the gpv's returned by GetPrimvars are REALLY valid,
        # and that the UsdAttribute metadata wrappers work
        self.assertEqual( datas[0].GetTypeName(), Sdf.ValueTypeNames.Matrix4dArray )
        self.assertEqual( datas[1].GetTypeName(), Sdf.ValueTypeNames.FloatArray )
        self.assertEqual( datas[2].GetBaseName(), "v_1" )
        
        # Now we'll add some extra configuration and verify that the 
        # interrogative API works properly
        self.assertEqual( u1.GetInterpolation(), UsdGeom.Tokens.constant )  # fallback
        self.assertFalse( u1.HasAuthoredInterpolation() )
        self.assertFalse( u1.HasAuthoredElementSize() )
        self.assertTrue( u1.SetInterpolation(UsdGeom.Tokens.vertex) )
        self.assertTrue( u1.HasAuthoredInterpolation() )
        self.assertEqual( u1.GetInterpolation(), UsdGeom.Tokens.vertex )

        self.assertFalse( v1.HasAuthoredInterpolation() )
        self.assertFalse( v1.HasAuthoredElementSize() )
        self.assertTrue( v1.SetInterpolation(UsdGeom.Tokens.uniform) )
        self.assertTrue( v1.SetInterpolation(UsdGeom.Tokens.varying) )
        self.assertTrue( v1.SetInterpolation(UsdGeom.Tokens.constant) )
        self.assertTrue( v1.SetInterpolation(UsdGeom.Tokens.faceVarying) )

        with self.assertRaises(Tf.ErrorException):
            v1.SetInterpolation("frobosity")
        # Should be the last good value set
        self.assertEqual( v1.GetInterpolation(), "faceVarying" )

        self.assertTrue( _3dpmats.HasAuthoredInterpolation() )
        self.assertTrue( _3dpmats.HasAuthoredElementSize() )
        with self.assertRaises(Tf.ErrorException):
            _3dpmats.SetElementSize(0)
        # Failure to set shouldn't change the state...
        self.assertTrue( _3dpmats.HasAuthoredElementSize() )
        self.assertTrue( _3dpmats.SetElementSize(nPasses) )
        self.assertTrue( _3dpmats.HasAuthoredElementSize() )

        # Make sure value Get/Set work
        self.assertEqual( u1.Get(), None )

        self.assertFalse(u1.IsIndexed())
        self.assertEqual(u1.ComputeFlattened(), None)

        uVal = Vt.FloatArray([1.1,2.1,3.1])
        self.assertTrue(u1.Set(uVal))
        self.assertEqual(u1.Get(), uVal)

        # Make sure indexed primvars work
        self.assertFalse(u1.IsIndexed())
        indices = Vt.IntArray([0, 1, 2, 2, 1, 0])
        self.assertTrue(u1.SetIndices(indices))
        self.assertTrue(u1.IsIndexed())

        self.assertEqual(u1.GetIndices(), indices)
        for a, b in zip(u1.ComputeFlattened(),
                        [1.1, 2.1, 3.1, 3.1, 2.1, 1.1]):
            self.assertTrue(Gf.IsClose(a, b, 1e-5))
        self.assertNotEqual(u1.ComputeFlattened(), u1.Get())

        self.assertEqual(u1.GetUnauthoredValuesIndex(), -1)
        self.assertTrue(u1.SetUnauthoredValuesIndex(2))
        self.assertEqual(u1.GetUnauthoredValuesIndex(), 2)

        self.assertEqual(u1.GetTimeSamples(), [])
        self.assertFalse(u1.ValueMightBeTimeVarying())

        indicesAt1 = Vt.IntArray([1,2,0])
        indicesAt2 = Vt.IntArray([])

        self.assertTrue(u1.SetIndices(indicesAt1, 1.0))
        self.assertEqual(u1.GetIndices(1.0), indicesAt1)

        uValAt1 = Vt.FloatArray([2.1,3.1,4.1])
        self.assertTrue(u1.Set(uValAt1, 1.0))
        self.assertEqual(u1.Get(1.0), uValAt1)

        self.assertEqual(u1.GetTimeSamples(), [1.0])
        self.assertFalse(u1.ValueMightBeTimeVarying())

        self.assertTrue(u1.SetIndices(indicesAt2, 2.0))
        self.assertEqual(u1.GetIndices(2.0), indicesAt2)

        uValAt2 = Vt.FloatArray([3.1,4.1,5.1])
        self.assertTrue(u1.Set(uValAt2, 2.0))
        self.assertEqual(u1.Get(2.0), uValAt2)

        self.assertEqual(u1.GetTimeSamples(), [1.0, 2.0])
        self.assertEqual(u1.GetTimeSamplesInInterval(Gf.Interval(0.5, 1.5)), [1.0])
        self.assertTrue(u1.ValueMightBeTimeVarying())

        for a, b in zip(u1.ComputeFlattened(1.0),
                        [3.1, 4.1, 2.1]):
            self.assertTrue(Gf.IsClose(a, b, 1e-5))
 
        self.assertNotEqual(u1.ComputeFlattened(1.0), u1.Get(1.0))

        self.assertTrue(len(u1.ComputeFlattened(2.0)) == 0)
        self.assertNotEqual(u1.ComputeFlattened(2.0), u1.Get(2.0))

        # Ensure that primvars with indices only authored at timeSamples
        # (i.e. no default) are recognized as such.  Manual name-munging
        # necessitated by UsdGeomPrimvar's lack of API for accessing
        # the indices attribute directly!
        u1Indices = p.GetAttribute(u1.GetName() + ":indices")
        self.assertTrue(u1Indices)
        u1Indices.ClearDefault()
        self.assertTrue(u1.IsIndexed())
        
        # Finally, ensure the values returned by GetDeclarationInfo
        # (on new Primvar objects, to test the GprimSchema API)
        # is identical to the individual queries, and matches what we set above
        nu1 = gp.GetPrimvar("u_1")
        (name, typeName, interpolation, elementSize) = nu1.GetDeclarationInfo()
        self.assertEqual(name,          u1.GetBaseName())
        self.assertEqual(typeName,      u1.GetTypeName())
        self.assertEqual(interpolation, u1.GetInterpolation())
        self.assertEqual(elementSize,   u1.GetElementSize())
        
        self.assertEqual(name,          "u_1")
        self.assertEqual(typeName,      Sdf.ValueTypeNames.FloatArray)
        self.assertEqual(interpolation, UsdGeom.Tokens.vertex)
        self.assertEqual(elementSize,   1)
        
        nv1 = gp.GetPrimvar("v_1")
        (name, typeName, interpolation, elementSize) = nv1.GetDeclarationInfo()
        self.assertEqual(name,          v1.GetBaseName())
        self.assertEqual(typeName,      v1.GetTypeName())
        self.assertEqual(interpolation, v1.GetInterpolation())
        self.assertEqual(elementSize,   v1.GetElementSize())
        
        self.assertEqual(name,          "v_1")
        self.assertEqual(typeName,      Sdf.ValueTypeNames.FloatArray)
        self.assertEqual(interpolation, UsdGeom.Tokens.faceVarying)
        self.assertEqual(elementSize,   1)

        nmats = gp.GetPrimvar('projMats')
        (name, typeName, interpolation, elementSize) = nmats.GetDeclarationInfo()
        self.assertEqual(name,          _3dpmats.GetBaseName())
        self.assertEqual(typeName,      _3dpmats.GetTypeName())
        self.assertEqual(interpolation, _3dpmats.GetInterpolation())
        self.assertEqual(elementSize,   _3dpmats.GetElementSize())
        
        self.assertEqual(name,          'projMats')
        self.assertEqual(typeName,      Sdf.ValueTypeNames.Matrix4dArray)
        self.assertEqual(interpolation, UsdGeom.Tokens.constant)
        self.assertEqual(elementSize,   nPasses)

        # Id primvar
        notId = gp.CreatePrimvar('notId', Sdf.ValueTypeNames.FloatArray)
        self.assertFalse(notId.IsIdTarget())
        with self.assertRaises(Tf.ErrorException):
            notId.SetIdTarget(gp.GetPath())

        handleid = gp.CreatePrimvar('handleid', Sdf.ValueTypeNames.String)

        # make sure we can still just set a string
        v = "handleid_value"
        self.assertTrue(handleid.Set(v))
        self.assertEqual(handleid.Get(), v)
        self.assertEqual(handleid.ComputeFlattened(), v)

        numPrimvars = len(gp.GetPrimvars())
        
        # This check below ensures that the "indices" attributes belonging to 
        # indexed primvars aren't considered to be primvars themselves.
        self.assertEqual(numPrimvars, 7)

        self.assertTrue(handleid.SetIdTarget(gp.GetPath()))
        # make sure we didn't increase the number of primvars (also that
        # GetPrimvars doesn't break when we have relationships)
        self.assertEqual(len(gp.GetPrimvars()), numPrimvars)
        self.assertEqual(handleid.Get(), gp.GetPath())

        stringPath = '/my/string/path'
        self.assertTrue(handleid.SetIdTarget(stringPath))
        self.assertEqual(handleid.Get(), Sdf.Path(stringPath))

        p = Sdf.Path('/does/not/exist')
        self.assertTrue(handleid.SetIdTarget(p))
        self.assertEqual(handleid.Get(), p)

        handleid_array = gp.CreatePrimvar('handleid_array', Sdf.ValueTypeNames.StringArray)
        self.assertTrue(handleid_array.SetIdTarget(gp.GetPath()))

    def test_Bug124579(self):
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
        self.assertEqual(primvar.Get(), Vt.StringArray(len(value), value))

    def test_PrimvarIndicesBlock(self):
        # We'll put all our Primvar on a single mesh gprim
        stage = Usd.Stage.CreateInMemory('indexPrimvars.usda')
        gp = UsdGeom.Mesh.Define(stage, '/myMesh')
        foo = gp.CreatePrimvar('foo', Sdf.ValueTypeNames.FloatArray)
        indices = Vt.IntArray([0, 1, 2, 2, 1, 0])
        self.assertTrue(foo.SetIndices(indices))
        self.assertTrue(foo.IsIndexed())
        foo.BlockIndices()
        self.assertFalse(foo.IsIndexed())

if __name__ == "__main__":
    unittest.main()
