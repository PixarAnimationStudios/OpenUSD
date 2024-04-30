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

from pxr import Sdf, Pcp
import unittest

testPaths = [
    Sdf.Path(),
    Sdf.Path('/'),
    Sdf.Path('/foo'),
    Sdf.Path('/a/b/c')
    ]

class TestPcpMapFunction(unittest.TestCase):

    def test_Null(self):
        # Test null function
        null = Pcp.MapFunction()
        self.assertTrue(null.isNull)
        self.assertFalse(null.isIdentity)
        self.assertFalse(null.isIdentityPathMapping)
        self.assertEqual(null.timeOffset, Sdf.LayerOffset())
        for path in testPaths:
            self.assertTrue(null.MapSourceToTarget( path ).isEmpty)

    def test_Identity(self):
        # Test identity function
        identity = Pcp.MapFunction.Identity()
        self.assertFalse(identity.isNull)
        self.assertTrue(identity.isIdentity)
        self.assertTrue(identity.isIdentityPathMapping)
        self.assertEqual(identity.timeOffset, Sdf.LayerOffset())
        for path in testPaths:
            self.assertEqual(identity.MapSourceToTarget(path), path)

        # Test identity path mapping with non-identity offset
        identityPathMapping = Pcp.MapFunction(Pcp.MapFunction.IdentityPathMap(),
                                              Sdf.LayerOffset(scale=10.0))
        self.assertFalse(identityPathMapping.isNull)
        self.assertFalse(identityPathMapping.isIdentity)
        self.assertTrue(identityPathMapping.isIdentityPathMapping)
        self.assertEqual(identityPathMapping.timeOffset,
                         Sdf.LayerOffset(scale=10.0))
        for path in testPaths:
            self.assertEqual(identityPathMapping.MapSourceToTarget(path), path)

    def test_Simple(self):
        # Test a simple mapping, simulating a referenced model instance.
        m = Pcp.MapFunction({'/Model': '/Model_1'})
        self.assertFalse(m.isNull)
        self.assertFalse(m.isIdentity)
        self.assertFalse(m.isIdentityPathMapping)
        self.assertTrue(m.MapSourceToTarget('/').isEmpty)
        self.assertEqual(m.MapSourceToTarget('/Model'), Sdf.Path('/Model_1'))
        self.assertEqual(
            m.MapSourceToTarget('/Model/anim'), Sdf.Path('/Model_1/anim'))
        self.assertTrue(m.MapSourceToTarget('/Model_1').isEmpty)
        self.assertTrue(m.MapSourceToTarget('/Model_1/anim').isEmpty)
        self.assertTrue(m.MapTargetToSource('/').isEmpty)
        self.assertTrue(m.MapTargetToSource('/Model').isEmpty)
        self.assertTrue(m.MapTargetToSource('/Model/anim').isEmpty)
        self.assertEqual(m.MapTargetToSource('/Model_1'), Sdf.Path('/Model'))
        self.assertEqual(
            m.MapTargetToSource('/Model_1/anim'), Sdf.Path('/Model/anim'))

        # Mapping functions do not affect nested target paths.
        self.assertEqual(m.MapTargetToSource('/Model_1.x[/Model_1.y]'),
                         Sdf.Path('/Model.x[/Model_1.y]'))
        self.assertEqual(m.MapSourceToTarget('/Model.x[/Model.y]'),
                         Sdf.Path('/Model_1.x[/Model.y]'))

    def test_NestedRef(self):
        # Test a mapping representing a nested rig reference.
        m2 = Pcp.MapFunction({'/CharRig': '/Model/Rig'})
        self.assertFalse(m2.isNull)
        self.assertFalse(m2.isIdentity)
        self.assertFalse(m2.isIdentityPathMapping)
        self.assertTrue(m2.MapSourceToTarget('/').isEmpty)
        self.assertEqual(m2.MapSourceToTarget('/CharRig'),
                         Sdf.Path('/Model/Rig'))
        self.assertEqual(m2.MapSourceToTarget('/CharRig/rig'),
                         Sdf.Path('/Model/Rig/rig'))
        self.assertTrue(m2.MapSourceToTarget('/Model').isEmpty)
        self.assertTrue(m2.MapSourceToTarget('/Model/Rig').isEmpty)
        self.assertTrue(m2.MapSourceToTarget('/Model/Rig/rig').isEmpty)
        self.assertTrue(m2.MapTargetToSource('/').isEmpty)
        self.assertTrue(m2.MapTargetToSource('/CharRig').isEmpty)
        self.assertTrue(m2.MapTargetToSource('/CharRig/rig').isEmpty)
        self.assertTrue(m2.MapTargetToSource('/Model').isEmpty)
        self.assertEqual(m2.MapTargetToSource('/Model/Rig'),
                         Sdf.Path('/CharRig'))
        self.assertEqual(m2.MapTargetToSource('/Model/Rig/rig'),
                         Sdf.Path('/CharRig/rig'))

    def test_Composition(self):
        # Test composing two map functions.
        m = Pcp.MapFunction({'/Model': '/Model_1'})
        m2 = Pcp.MapFunction({'/CharRig': '/Model/Rig'})
        m3 = m.Compose(m2)
        self.assertFalse(m3.isNull)
        self.assertFalse(m3.isIdentity)
        self.assertFalse(m3.isIdentityPathMapping)
        self.assertTrue(m3.MapSourceToTarget('/').isEmpty)
        self.assertEqual(m3.MapSourceToTarget('/CharRig'),
                         Sdf.Path('/Model_1/Rig'))
        self.assertEqual(m3.MapSourceToTarget('/CharRig/rig'), 
                         Sdf.Path('/Model_1/Rig/rig'))
        self.assertTrue(m3.MapSourceToTarget('/Model').isEmpty)
        self.assertTrue(m3.MapSourceToTarget('/Model/Rig').isEmpty)
        self.assertTrue(m3.MapSourceToTarget('/Model/Rig/rig').isEmpty)
        self.assertTrue(m3.MapSourceToTarget('/Model_1').isEmpty)
        self.assertTrue(m3.MapSourceToTarget('/Model_1/Rig').isEmpty)
        self.assertTrue(m3.MapSourceToTarget('/Model_1/Rig/rig').isEmpty)
        self.assertTrue(m3.MapTargetToSource('/').isEmpty)
        self.assertTrue(m3.MapTargetToSource('/CharRig').isEmpty)
        self.assertTrue(m3.MapTargetToSource('/CharRig/rig').isEmpty)
        self.assertTrue(m3.MapTargetToSource('/Model').isEmpty)
        self.assertTrue(m3.MapTargetToSource('/Model/Rig').isEmpty)
        self.assertTrue(m3.MapTargetToSource('/Model/Rig/rig').isEmpty)
        self.assertTrue(m3.MapTargetToSource('/Model_1').isEmpty)
        self.assertEqual(m3.MapTargetToSource('/Model_1/Rig'),
                         Sdf.Path('/CharRig'))
        self.assertEqual(m3.MapTargetToSource('/Model_1/Rig/rig'),
                         Sdf.Path('/CharRig/rig'))
        # Test composing map functions that should produce identity mappings.
        m1 = Pcp.MapFunction({'/':'/', '/a':'/b'})
        m2 = Pcp.MapFunction({'/':'/', '/b':'/a'})
        self.assertEqual(m1.Compose(m2), Pcp.MapFunction.Identity())
        self.assertEqual(m2.Compose(m1), Pcp.MapFunction.Identity())

    def test_InheritRelocateChain(self):
        # Test a chain of composed mappings that simulates an inherit of
        # a local class, along with some relocations:
        #
        # - M_1 is an instance of the model, M
        # - M/Rig/Inst is an instance of the local class M/Rig/Class
        # - M/Rig/Inst/Scope is an anim scope relocated to M/Anim/Scope
        #
        m4 = Pcp.MapFunction({'/M': '/M_1'} ).Compose(
            Pcp.MapFunction( {'/M/Rig/Inst/Scope': '/M/Anim/Scope'} ).Compose(
                Pcp.MapFunction( {'/M/Rig/Class': '/M/Rig/Inst'} ).Compose(
                    Pcp.MapFunction( {'/M_1': '/M'} ))))

        # The composed result should map opinions from the model instance's
        # rig class scope, to the relocated anim scope.
        expected = Pcp.MapFunction({'/M_1/Rig/Class/Scope': '/M_1/Anim/Scope'})
        self.assertEqual(m4, expected)
        self.assertTrue(m4.MapSourceToTarget(
            Sdf.Path('/M_1/Rig/Class/Scope/x')) == Sdf.Path('/M_1/Anim/Scope/x'))
        self.assertTrue(m4.MapTargetToSource(
            Sdf.Path('/M_1/Anim/Scope/x')) == Sdf.Path('/M_1/Rig/Class/Scope/x'))

    def test_LayerOffsets(self):
        # Test layer offsets
        offset1 = Sdf.LayerOffset(offset=0.0, scale=2.0)
        offset2 = Sdf.LayerOffset(offset=10.0, scale=1.0)
        m5 = Pcp.MapFunction({'/':'/'}, offset1)
        m6 = Pcp.MapFunction({'/':'/'}, offset2)

        self.assertEqual(m5.timeOffset, offset1)
        self.assertEqual(m6.timeOffset, offset2)
        self.assertEqual(m5.Compose(m6).timeOffset, (offset1 * offset2))
        self.assertEqual(m5.ComposeOffset(m6.timeOffset).timeOffset, (offset1 * offset2))

    def test_Basics(self):
        testMapFuncs = [
            Pcp.MapFunction(),
            Pcp.MapFunction.Identity(),
            Pcp.MapFunction(Pcp.MapFunction.IdentityPathMap(),
                            Sdf.LayerOffset(scale=10.0)),
            Pcp.MapFunction({'/Model': '/Model_1'}),
            Pcp.MapFunction({'/CharRig': '/Model/Rig'}),
            Pcp.MapFunction({'/Model': '/Model_1'}).Compose(
                Pcp.MapFunction({'/CharRig': '/Model/Rig'})),
            Pcp.MapFunction({'/M': '/M_1'} ).Compose(
                Pcp.MapFunction(
                    {'/M/Rig/Inst/Scope': '/M/Anim/Scope'} ).Compose(
                        Pcp.MapFunction(
                            {'/M/Rig/Class': '/M/Rig/Inst'} ).Compose(
                                Pcp.MapFunction( {'/M_1': '/M'} )))),
            Pcp.MapFunction({'/':'/'}, Sdf.LayerOffset(offset=0.0, scale=2.0)),
            Pcp.MapFunction({'/':'/'}, Sdf.LayerOffset(offset=10.0, scale=1.0))
        ]
        
        # Test equality/inequality
        for i in range(len(testMapFuncs)):
            for j in range(len(testMapFuncs)):
                if i == j:
                    self.assertEqual(testMapFuncs[i], testMapFuncs[j])
                else:
                    self.assertNotEqual(testMapFuncs[i], testMapFuncs[j])

        # Test repr
        for m in testMapFuncs:
            self.assertEqual(eval(repr(m)), m)

        # Composing any function with identity should return itself.
        identity = Pcp.MapFunction.Identity()
        for mapFunc in testMapFuncs:
            self.assertEqual(mapFunc.Compose(identity), mapFunc)
            self.assertEqual(identity.Compose(mapFunc), mapFunc)

        # Composing a function that has an identity offset with a null function 
        # should return null. 
        # Composing a function that has an offset with a null function 
        # should just return that offset.
        for mapFunc in testMapFuncs:
            null = Pcp.MapFunction()
            if mapFunc.timeOffset.IsIdentity():
                self.assertEqual(mapFunc.Compose(null), null)
                self.assertEqual(null.Compose(mapFunc), null)
            else:
                self.assertEqual(mapFunc.Compose(null),
                                 Pcp.MapFunction({}, mapFunc.timeOffset))
                self.assertEqual(null.Compose(mapFunc),
                                 Pcp.MapFunction({}, mapFunc.timeOffset))

    def test_Canonicalization(self):
        # Test map function canonicalization.
        self.assertEqual(Pcp.MapFunction({}), Pcp.MapFunction({}))
        self.assertEqual(Pcp.MapFunction({'/A':'/A'}),
                         Pcp.MapFunction({'/A':'/A'}))
        self.assertTrue(Pcp.MapFunction({'/':'/', '/A':'/A'}) ==
                        Pcp.MapFunction({'/':'/'}))
        self.assertTrue(Pcp.MapFunction({'/A':'/B', '/A/X':
                                         '/B/X', '/A/X/Y1':'/B/X/Y2'}) ==
                        Pcp.MapFunction({'/A':'/B', '/A/X/Y1':'/B/X/Y2'}))

    # Test case for bug 74847.
    def test_Bug74847(self):
        m = Pcp.MapFunction({'/A': '/A/B'})
        self.assertEqual(m.MapSourceToTarget('/A/B'), Sdf.Path('/A/B/B'))
        self.assertEqual(m.MapTargetToSource('/A/B/B'), Sdf.Path('/A/B'))

    # Test case for bug 112645. See also TrickyInheritsAndRelocates3 test
    # in testPcpMuseum.
    def test_Bug112645(self):
        f1 = Pcp.MapFunction(
            {'/GuitarRig':'/GuitarRigX',
             '/GuitarRig/Rig/StringsRig/_Class_StringRig/String':
             '/GuitarRigX/Anim/Strings/String1'})

        f2 = Pcp.MapFunction(
            {'/StringsRig/String1Rig/String' :
             '/GuitarRig/Anim/Strings/String1',
             '/StringsRig' :
             '/GuitarRig/Rig/StringsRig'})

        self.assertEqual(f1.Compose(f2), Pcp.MapFunction({
            '/StringsRig':'/GuitarRigX/Rig/StringsRig',
            '/StringsRig/_Class_StringRig/String':
            '/GuitarRigX/Anim/Strings/String1'}))

    # Test for a bug where the composed map function of two map functions does not
    # produce the same MapSourceToTarget(path) results as passing the path through
    # the two map functions individually.
    #
    # XXX: As of now this bug is not fixed.
    def test_BugComposedMapFunction(self):
        # The first map function maps /PathRig to a target path and also maps
        # a /PathRig/Path (a child of /PathRig) to a path that is not a descendant 
        # of its parent's mapped target.
        f1 = Pcp.MapFunction({
            '/PathRig' : '/CharRig/Rig/PathRig',
            '/PathRig/Path' : '/Path'
        })

        # The second map function is maps an ancestor of one entry's target in the
        # first map function but does not map both.
        f2 = Pcp.MapFunction({
            '/CharRig' : '/Model'
        })

        # Compose the map functions for the equivalent of mapping f2(f1(path))
        fComposed = f2.Compose(f1)

        # Verify the resulting composed function.
        #
        # XXX: This composed function is NOT the equivalent of calling f2(f1(path))
        # for all paths and is the crux of the bug.
        self.assertEqual(fComposed, Pcp.MapFunction({
            '/PathRig' : '/Model/Rig/PathRig'
        }))

        # Verifies that calling the composed map function on a path produces the
        # same result as calling its composing map functions in sequence on the 
        # the same path
        def _VerifyComposedFunctionMapsSourceToTargetTheSame(sourcePath,targetPath):
            # The composed functions MapSourceToTarget will produce the same path
            # as calling f1.MapSourceToTarget followed by f2.MapSourceToTarget
            self.assertEqual(
                fComposed.MapSourceToTarget(sourcePath), 
                targetPath)
            self.assertEqual(
                f2.MapSourceToTarget(f1.MapSourceToTarget(sourcePath)), 
                targetPath)

        def _VerifyComposedFunctionMapsTargetToSourceTheSame(targetPath,sourcePath):
            # For MapTargetToSource, it's inverted. The composed MapTargetToSource
            # is the same as calling f2.MapTargetToSource followed by 
            # f1.MapTargetToSource
            self.assertEqual(
                fComposed.MapTargetToSource(targetPath), 
                sourcePath)
            self.assertEqual(
                f1.MapTargetToSource(f2.MapTargetToSource(targetPath)),
                sourcePath)

        # Verify path that fails to map in f1.
        _VerifyComposedFunctionMapsSourceToTargetTheSame('/Bogus', '')
        _VerifyComposedFunctionMapsTargetToSourceTheSame('/Bogus', '')

        # Verify path that is the first direct source in f1. This will map
        _VerifyComposedFunctionMapsSourceToTargetTheSame(
            '/PathRig', '/Model/Rig/PathRig')
        _VerifyComposedFunctionMapsTargetToSourceTheSame(
            '/Model/Rig/PathRig', '/PathRig')

        # Verify path that is a child of the first direct source 
        _VerifyComposedFunctionMapsSourceToTargetTheSame(
            '/PathRig/Rig', '/Model/Rig/PathRig/Rig')
        _VerifyComposedFunctionMapsTargetToSourceTheSame(
            '/Model/Rig/PathRig/Rig', '/PathRig/Rig')

        # Verify path that is the second direct source in f1 will fail to map
        # across f2 and therefore will also fail to map across the composed
        # function.
        #
        # XXX: This currently cannot be verified because the composed function is 
        # wrong. For now we verify the incorrect behavior until the bug is fixed.
        #
        # _VerifyComposedFunctionMapsSourceToTargetTheSame('/PathRig/Path', '')
        self.assertEqual(
            f2.MapSourceToTarget(f1.MapSourceToTarget('/PathRig/Path')),
            Sdf.Path())
        self.assertEqual(
            fComposed.MapSourceToTarget('/PathRig/Path'),
            '/Model/Rig/PathRig/Path')

        # Verify target path that can inverse map across f2 but fails to inverse
        # map across f1 will also fail to inverse map across the composed function.
        #
        # XXX: This currently cannot be verified because the composed function is 
        # wrong. For now we verify the incorrect behavior until the bug is fixed.
        #
        # _VerifyComposedFunctionMapsTargetToSourceTheSame(
        #     '/Model/Rig/PathRig/Path', '')
        self.assertEqual(
            f1.MapTargetToSource(f2.MapTargetToSource('/Model/Rig/PathRig/Path')),
            Sdf.Path())
        self.assertEqual(
            fComposed.MapTargetToSource('/Model/Rig/PathRig/Path'),
            '/PathRig/Path')

if __name__ == "__main__":
    unittest.main()
