#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

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

    # Test map functions with explicit block.
    def test_MapFunctionsWithBlocks(self):

        # Function is identity mapping with an explicit block of /Model
        f = Pcp.MapFunction({
            '/' : '/',
            '/Model' : Sdf.Path()
        })

        # Non /Model paths map to themselves in both directions.
        self.assertEqual(f.MapSourceToTarget('/foo'), '/foo')
        self.assertEqual(f.MapTargetToSource('/foo'), '/foo')
        self.assertEqual(f.MapSourceToTarget('/foo/bar'), '/foo/bar')
        self.assertEqual(f.MapTargetToSource('/foo/bar'), '/foo/bar')
        # Any paths at or under /Model do not map in either direction.
        self.assertEqual(f.MapSourceToTarget('/Model'), Sdf.Path())
        self.assertEqual(f.MapTargetToSource('/Model'), Sdf.Path())
        self.assertEqual(f.MapSourceToTarget('/Model/Bar'), Sdf.Path())
        self.assertEqual(f.MapTargetToSource('/Model/Bar'), Sdf.Path())

        # Function renames /CharRig, blocks /CharRig/Rig, and moves 
        # /CharRig/Rig/Anim
        f = Pcp.MapFunction({
            '/CharRig' : '/Char',
            '/CharRig/Rig' : Sdf.Path(),
            '/CharRig/Rig/Anim' : '/Char/Anim'
        })

        # CharRig and its descendants (besides Rig) forward map to /Char
        self.assertEqual(f.MapSourceToTarget('/CharRig'), '/Char')
        self.assertEqual(f.MapSourceToTarget('/CharRig/Foo'), '/Char/Foo')
        # /CharRig/Rig and its descendants (besides Anim) do not forward map
        self.assertEqual(f.MapSourceToTarget('/CharRig/Rig'), '')
        self.assertEqual(f.MapSourceToTarget('/CharRig/Rig/Foo'), '')
        # /CharRig/Rig/Anim and its descendants forward map to /Char/Anim
        self.assertEqual(f.MapSourceToTarget('/CharRig/Rig/Anim'), '/Char/Anim')
        self.assertEqual(f.MapSourceToTarget('/CharRig/Rig/Anim/Foo'), 
                         '/Char/Anim/Foo')

        # Reverse mapping /Char to /CharRig works.
        self.assertEqual(f.MapTargetToSource('/Char'), '/CharRig')
        self.assertEqual(f.MapTargetToSource('/Char/Foo'), '/CharRig/Foo')
        # Reverse mapping of /Char/Rig fails as the block prevents any paths
        # from forward mapping to /Char/Rig.
        self.assertEqual(f.MapTargetToSource('/Char/Rig'), '')
        self.assertEqual(f.MapTargetToSource('/Char/Rig/Foo'), '')
        self.assertEqual(f.MapTargetToSource('/Char/Rig/Anim'), '')
        # Reverse mapping from /Char/Anim to /CharRig/Rig/Anim works.
        self.assertEqual(f.MapTargetToSource('/Char/Anim'), '/CharRig/Rig/Anim')
        self.assertEqual(f.MapTargetToSource('/Char/Anim/Foo'), 
                         '/CharRig/Rig/Anim/Foo')

    def test_Canonicalization(self):
        # Test map function canonicalization.
        self.assertEqual(Pcp.MapFunction({}).sourceToTargetMap, {})
        self.assertEqual(Pcp.MapFunction({'/A':'/A'}).sourceToTargetMap,
                         {Sdf.Path('/A') : Sdf.Path('/A')})

        # / -> / makes /A -> /A redundant but not /A -> /B        
        self.assertEqual(Pcp.MapFunction({'/':'/', '/A':'/A'}).sourceToTargetMap,
                         {Sdf.Path('/') : Sdf.Path('/')})
        self.assertEqual(Pcp.MapFunction({'/':'/', '/A':'/B'}).sourceToTargetMap,
                         {Sdf.Path('/') : Sdf.Path('/'),
                          Sdf.Path('/A') : Sdf.Path('/B')})
        
        # /A -> /B makes /A/X -> /B/X redundant, but /A/X/Y1 -> /B/X/Y2 is not
        # redundant.
        self.assertEqual(Pcp.MapFunction({'/A':'/B', 
                                          '/A/X':'/B/X', 
                                          '/A/X/Y1':'/B/X/Y2'}).sourceToTargetMap,
                         {Sdf.Path('/A') : Sdf.Path('/B'), 
                          Sdf.Path('/A/X/Y1') : Sdf.Path('/B/X/Y2')})
        
        # /A -> /B makes /A/X1/C -> /B/X1/C redundant
        self.assertEqual(Pcp.MapFunction({'/A':'/B', 
                                          '/A/X1/C':'/B/X1/C'}).sourceToTargetMap,
                         {Sdf.Path('/A') : Sdf.Path('/B')})
        # But adding /A/X1 -> /B/X2 makes /A/X1/C -> /B/X1/C no longer redundant
        # as it is a closer ancestor mapping than /A -> /B
        self.assertEqual(Pcp.MapFunction({'/A':'/B', 
                                          '/A/X1':'/B/X2', 
                                          '/A/X1/C':'/B/X1/C'}).sourceToTargetMap,
                         {Sdf.Path('/A') : Sdf.Path('/B'), 
                          Sdf.Path('/A/X1') : Sdf.Path('/B/X2'), 
                          Sdf.Path('/A/X1/C') : Sdf.Path('/B/X1/C')})

        # Mapping /A -> empty with no other mappings is redundant as there are 
        # no mappings to begin with.
        self.assertEqual(Pcp.MapFunction({'/A': ''}).sourceToTargetMap, {})
        # But adding the identity mapping does make /A -> empty relevant to 
        # unmap the mapping of /A
        self.assertEqual(Pcp.MapFunction({'/': '/', '/A': ''}).sourceToTargetMap,
                         {Sdf.Path('/') : Sdf.Path('/'),
                          Sdf.Path('/A') : Sdf.Path()})
        
        # /A/B -> /A/B is not redundant even in the face of / -> / as the 
        # unmapping of its closer ancestor, /A -> empty, makes it relevant.
        self.assertEqual(Pcp.MapFunction({'/': '/', 
                                          '/A': '',
                                          '/A/B': '/A/B'}).sourceToTargetMap,
                         {Sdf.Path('/') : Sdf.Path('/'),
                          Sdf.Path('/A') : Sdf.Path(),
                          Sdf.Path('/A/B') : Sdf.Path('/A/B')})
        
        # The empty mappings of descendants of /A are redundant because /A
        # is already unmapped.
        self.assertEqual(Pcp.MapFunction({'/': '/', 
                                          '/A': '',
                                          '/A/B/C': '',
                                          '/A/D': ''}).sourceToTargetMap,
                         {Sdf.Path('/') : Sdf.Path('/'),
                          Sdf.Path('/A') : Sdf.Path()})
        # But adding a mapping of /A/B -> /A/B now makes the /A/B/C -> empty
        # map relevant. /A/D -> empty is still redundant since its closest
        # ancestor is /A.
        self.assertEqual(Pcp.MapFunction({'/': '/', 
                                          '/A': '',
                                          '/A/B': '/A/B',
                                          '/A/B/C': '',
                                          '/A/D': ''}).sourceToTargetMap,
                         {Sdf.Path('/') : Sdf.Path('/'),
                          Sdf.Path('/A') : Sdf.Path(),
                          Sdf.Path('/A/B') : Sdf.Path('/A/B'),
                          Sdf.Path('/A/B/C') : Sdf.Path()})

        # The mapping of /B/C/E -> /A/C/E is redundant because of /B -> /A.
        self.assertEqual(Pcp.MapFunction({'/B': '/A', 
                                          '/B/C/E': '/A/C/E'}).sourceToTargetMap,
                         {Sdf.Path('/B') : Sdf.Path('/A')})
        # But adding an intermediary mapping of /B/C -> /C makes 
        # /B/C/E -> /A/C/E no longer redundant.
        self.assertEqual(Pcp.MapFunction({'/B': '/A', 
                                          '/B/C': '/C',
                                          '/B/C/E': '/A/C/E'}).sourceToTargetMap,
                         {Sdf.Path('/B') : Sdf.Path('/A'),
                          Sdf.Path('/B/C') : Sdf.Path('/C'),
                          Sdf.Path('/B/C/E') : Sdf.Path('/A/C/E')})

        # Create the inverses of both above mappings. Note that the /C - /B/C
        # mapping makes /A/C/E -> /B/C/E not redundant because of how it affects
        # inverses of these functions.
        self.assertEqual(Pcp.MapFunction({'/A': '/B', 
                                          '/A/C/E': '/B/C/E'}).sourceToTargetMap,
                         {Sdf.Path('/A') : Sdf.Path('/B')})
        self.assertEqual(Pcp.MapFunction({'/A': '/B', 
                                          '/C': '/B/C',
                                          '/A/C/E': '/B/C/E'}).sourceToTargetMap,
                         {Sdf.Path('/A') : Sdf.Path('/B'),
                          Sdf.Path('/A/C/E') : Sdf.Path('/B/C/E'),
                          Sdf.Path('/C') : Sdf.Path('/B/C')})

        # The /Bar -> empty mapping is redundant because /Bar is already not 
        # mappable because it breaks bijection (/Bar would forward map to /Bar 
        # but would inverse map to /Foo).
        self.assertEqual(Pcp.MapFunction({'/': '/',
                                          '/Foo': '/Bar',
                                          '/Bar': ''}).sourceToTargetMap,
                         {Sdf.Path('/'): Sdf.Path('/'),
                          Sdf.Path('/Foo'): Sdf.Path('/Bar')})

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
            '/StringsRig/String1Rig/String':'',
            '/StringsRig/_Class_StringRig/String':
                '/GuitarRigX/Anim/Strings/String1'}))

    # Test for a bug where the composed map function of two map functions would
    # not produce the same MapSourceToTarget(path) results as passing the path
    # through the two map functions individually.
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
        self.assertEqual(fComposed, Pcp.MapFunction({
            '/PathRig' : '/Model/Rig/PathRig',
            '/PathRig/Path' : Sdf.Path()
        }))

        # Verifies that calling the composed map function on a path produces the
        # same result as calling its composing map functions in sequence on the 
        # the same path
        def _VerifyComposedFunctionMapsSourceToTargetTheSame(
                sourcePath, targetPath):
            # The composed functions MapSourceToTarget will produce the same path
            # as calling f1.MapSourceToTarget followed by f2.MapSourceToTarget
            self.assertEqual(
                fComposed.MapSourceToTarget(sourcePath), 
                targetPath)
            self.assertEqual(
                f2.MapSourceToTarget(f1.MapSourceToTarget(sourcePath)), 
                targetPath)

        def _VerifyComposedFunctionMapsTargetToSourceTheSame(
                targetPath, sourcePath):
            # For MapTargetToSource, it's inverted. The composed 
            # MapTargetToSource is the same as calling f2.MapTargetToSource 
            # followed by f1.MapTargetToSource
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
        _VerifyComposedFunctionMapsSourceToTargetTheSame('/PathRig/Path', '')

        # Verify target path that can inverse map across f2 but fails to inverse
        # map across f1 will also fail to inverse map across the composed
        # function.
        _VerifyComposedFunctionMapsTargetToSourceTheSame(
            '/Model/Rig/PathRig/Path', '')

    def test_PathExpression(self):
        # Test mapping path expressions.
        m = Pcp.MapFunction({'/Model': '/Model_1'})
        pe = Sdf.PathExpression('//{shiny} /Model/Tee /Outside/the/model')
        mappedPe = m.MapSourceToTarget(pe)
        self.assertEqual(mappedPe, Sdf.PathExpression('//{shiny} /Model_1/Tee'))

if __name__ == "__main__":
    unittest.main()
