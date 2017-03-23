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
from pxr import Sdf, Tf

class TestSdfPath(unittest.TestCase):
    def test_Basic(self):
        # ========================================================================
        # Test SdfPath creation and pathString
        # ========================================================================
        
        print '\nTest creating bad paths: warnings expected'
        
        # XXX: Here are a couple bad paths that are 
        # currently allowed...  add these to the test cases when they are properly 
        # disallowed
        #  ../../
        #  .rel[targ][targ].attr
        #  .attr[1, 2, 3].attr
        
        badPaths = '''
            DD/DDD.&ddf$
            DD[]/DDD
            DD[]/DDD.bar
            foo.prop/bar
            /foo.prop/bar.blah
            /foo.prop/bar.blah
            /foo//bar
            /foo/.bar
            /foo..bar
            /foo.bar.baz
            /.foo
            </foo.bar
            </Foo/Bar/>
            /Foo/Bar/
            /Foo.bar[targ]/Bar
            /Foo.bar[targ].foo.foo
            123
            123test
            /Foo:Bar
            /Foo.bar.mapper[/Targ.attr].arg:name:space
            '''.split()
        for badPath in badPaths:
            self.assertTrue(Sdf.Path(badPath).isEmpty)
            self.assertEqual(Sdf.Path(badPath), Sdf.Path())
            self.assertEqual(Sdf.Path(badPath), Sdf.Path.emptyPath)
            self.assertFalse(Sdf.Path.IsValidPathString(badPath))
        print '\tPassed'
        
        # Test lessthan
        self.assertTrue(Sdf.Path('aaa') < Sdf.Path('aab'))
        self.assertTrue(not Sdf.Path('aaa') < Sdf.Path())
        self.assertTrue(Sdf.Path('/') < Sdf.Path('/a'))
        
        # XXX test path from elements ['.prop'] when we have that wrapped?
        
        # ========================================================================
        # Test SdfPath other queries
        # ========================================================================
        
        print '\nTest scenepath queries'
        testPathStrings = [
            "/Foo/Bar.baz",
            "Foo",
            "Foo/Bar",
            "Foo.bar",
            "Foo/Bar.bar",
            ".bar",
            "/Some/Kinda/Long/Path/Just/To/Make/Sure",
            "Some/Kinda/Long/Path/Just/To/Make/Sure.property",
            "../Some/Kinda/Long/Path/Just/To/Make/Sure",
            "../../Some/Kinda/Long/Path/Just/To/Make/Sure.property",
            "/Foo/Bar.baz[targ].boom",
            "Foo.bar[targ].boom",
            ".bar[targ].boom",
            "Foo.bar[targ.attr].boom",
            "/A/B/C.rel3[/Blah].attr3",
            "A/B.rel2[/A/B/C.rel3[/Blah].attr3].attr2",
            "/A.rel1[/A/B.rel2[/A/B/C.rel3[/Blah].attr3].attr2].attr1"
        ]
        testAbsolute = [True, False, False, False, False, False, True, False, False, False, True, False, False, False, True, False, True]
        testProperty = [True, False, False, True, True, True, False, True, False, True, True, True, True, True, True, True, True]
        testElements = [
            ["Foo", "Bar", ".baz"],
            ["Foo"],
            ["Foo", "Bar"],
            ["Foo", ".bar"],
            ["Foo", "Bar", ".bar"],
            [".bar"],
            ["Some", "Kinda", "Long", "Path", "Just", "To", "Make", "Sure"],
            ["Some", "Kinda", "Long", "Path", "Just", "To", "Make", "Sure", ".property"],
            ["..", "Some", "Kinda", "Long", "Path", "Just", "To", "Make", "Sure"],
            ["..", "..", "Some", "Kinda", "Long", "Path", "Just", "To", "Make", "Sure", ".property"],
            ["Foo", "Bar", ".baz", "[targ]", ".boom"],
            ["Foo", ".bar", "[targ]", ".boom"],
            [".bar", "[targ]", ".boom"],
            ["Foo", ".bar", "[targ.attr]", ".boom"],
            ["A", "B", "C", ".rel3", "[/Blah]", ".attr3"],
            ["A", "B", ".rel2", "[/A/B/C.rel3[/Blah].attr3]", ".attr2"],
            ["A", ".rel1", "[/A/B.rel2[/A/B/C.rel3[/Blah].attr3].attr2]", ".attr1"]
        ]
        
        # Test IsAbsolutePath and IsPropertyPath
        def BasicTest(path, elements):
            assert path.IsAbsolutePath() == testAbsolute[testIndex]
            assert path.IsPropertyPath() == testProperty[testIndex]
            prefixes = Sdf._PathElemsToPrefixes(path.IsAbsolutePath(), elements)
            assert path.GetPrefixes() == prefixes
            assert path == eval(repr(path))
            assert Sdf.Path.IsValidPathString(str(path))
        
        testPaths = list()
        for testIndex in range(len(testPathStrings)):
            string = testPathStrings[testIndex]
        
            # Test path
            testPaths.append(Sdf.Path(string))
            BasicTest(testPaths[-1], testElements[testIndex])
        
            # If path is a property then try it with a namespaced name.
            if testProperty[testIndex]:
                testPaths.append(Sdf.Path(string + ':this:has:namespaces'))
                elements = list(testElements[testIndex])
                elements[-1] += ':this:has:namespaces'
                BasicTest(testPaths[-1], elements)
        
        print '\tPassed'
        
        # ========================================================================
        # Test SdfPath hashing
        # ========================================================================
        dict = {}
        for i in enumerate(testPaths):
            dict[i[1]] = i[0]
        for i in enumerate(testPaths):
            self.assertEqual(dict[i[1]], i[0])
        
        # ========================================================================
        # Test SdfPath <-> string conversion
        # ========================================================================
        
        self.assertEqual(Sdf.Path('foo'), 'foo')
        self.assertEqual('foo', Sdf.Path('foo'))
        
        self.assertNotEqual(Sdf.Path('foo'), 'bar')
        self.assertNotEqual('bar', Sdf.Path('foo'))
        
        # Test repr w/ quotes
        pathWithQuotes = Sdf.Path("b'\"")
        self.assertEqual( eval(repr(pathWithQuotes)), pathWithQuotes )
        
        # ========================================================================
        # Test SdfPath -> bool conversion
        # ========================================================================
        
        # Empty paths should evaluate to false, other paths should evaluate to true.
        self.assertTrue(Sdf.Path('/foo.bar[baz]'))
        self.assertTrue(Sdf.Path('/foo.bar'))
        self.assertTrue(Sdf.Path('/'))
        self.assertTrue(Sdf.Path('.'))
        self.assertTrue(not Sdf.Path())
        
        # For now, creating a path with Sdf.Path('') emits a warning and returns 
        # the empty path.
        warnings = []
        def _HandleWarning(notice, sender):
            warnings.append(notice.warning)
        
        listener = Tf.Notice.RegisterGlobally(Tf.DiagnosticNotice.IssuedWarning,
                                              _HandleWarning)

        self.assertFalse(Sdf.Path(''))
        
        # ========================================================================
        # Test converting relative paths to absolute paths
        # ========================================================================
        print "Test converting relative paths to absolute paths"
        
        anchor = Sdf.Path("/A/B/E/F/G")
        relPath = Sdf.Path("../../../C/D")
        self.assertEqual(relPath.MakeAbsolutePath( anchor ), "/A/B/C/D")

        # Try too many ".."s for the base path
        self.assertEqual(relPath.MakeAbsolutePath( "/A" ), Sdf.Path.emptyPath)
        
        relPath = Sdf.Path("../../../..") 
        self.assertEqual(relPath.MakeAbsolutePath( anchor ), "/A")
        
        relPath = Sdf.Path("../../.radius")
        self.assertEqual(relPath.MakeAbsolutePath( anchor ), "/A/B/E.radius")
        
        relPath = Sdf.Path("..")
        self.assertEqual(relPath.MakeAbsolutePath( Sdf.Path( "../../A/B") ), Sdf.Path.emptyPath)
        
        # test passing a property path as the anchor
        self.assertEqual(relPath.MakeAbsolutePath( Sdf.Path( "/A/B.radius") ), Sdf.Path.emptyPath)
        
        # test on an absolute path
        self.assertEqual(anchor.MakeAbsolutePath( anchor ), anchor)
        print '\tPassed'
        
        # ========================================================================
        # Test converting absolute paths to relative paths
        # ========================================================================
        print "Test converting absolute paths to relative paths"
        
        anchor = Sdf.Path("/A/B/E/F/G")
        absPath = Sdf.Path("/A/B/C/D")
        self.assertEqual(absPath.MakeRelativePath( anchor ), "../../../C/D")
        
        absPath = Sdf.Path("/H/I/J")
        self.assertEqual(absPath.MakeRelativePath( anchor ), "../../../../../H/I/J")
        
        absPath = Sdf.Path("/A/B/E/F/G/H/I/J.radius")
        self.assertEqual(absPath.MakeRelativePath( anchor ), "H/I/J.radius")
        
        anchor = Sdf.Path("/A/B")
        absPath = Sdf.Path("/A.radius")
        self.assertEqual( absPath.MakeRelativePath( anchor ), "../.radius" )
        
        self.assertEqual(absPath.MakeRelativePath( Sdf.Path("H/I") ), "")
        
        # test passing a property path as the anchor
        self.assertEqual(absPath.MakeRelativePath( Sdf.Path( "/A/B.radius") ), "")
        
        print '\tPassed'
        
        # ========================================================================
        # Test converting sub-optimal relative paths to optimal relative paths
        # ========================================================================
        print "Test converting sub-optimal relative paths to optimal relative paths"
        
        anchor = Sdf.Path("/A/B/C")
        relPath = Sdf.Path("../../B/C/D")
        self.assertEqual(relPath.MakeRelativePath( anchor ), "D")
        
        relPath = Sdf.Path("../../../A")
        self.assertEqual(relPath.MakeRelativePath( anchor ), "../..")
        print '\tPassed'
        
        # ========================================================================
        # Test GetPrimPath
        # ========================================================================
        print "Test GetPrimPath"
        
        primPath = Sdf.Path("/A/B/C").GetPrimPath()
        self.assertEqual(primPath, Sdf.Path("/A/B/C"))
        
        primPath = Sdf.Path("/A/B/C.foo").GetPrimPath()
        self.assertEqual(primPath, Sdf.Path("/A/B/C"))
        primPath = Sdf.Path("/A/B/C.foo:bar:baz").GetPrimPath()
        self.assertEqual(primPath, Sdf.Path("/A/B/C"))
        
        primPath = Sdf.Path("/A/B/C.foo[target].bar").GetPrimPath()
        self.assertEqual(primPath, Sdf.Path("/A/B/C"))
        primPath = Sdf.Path("/A/B/C.foo[target].bar:baz").GetPrimPath()
        self.assertEqual(primPath, Sdf.Path("/A/B/C"))
        
        primPath = Sdf.Path("A/B/C.foo[target].bar").GetPrimPath()
        self.assertEqual(primPath, Sdf.Path("A/B/C"))
        primPath = Sdf.Path("A/B/C.foo[target].bar:baz").GetPrimPath()
        self.assertEqual(primPath, Sdf.Path("A/B/C"))
        
        primPath = Sdf.Path("../C.foo").GetPrimPath()
        self.assertEqual(primPath, Sdf.Path("../C"))
        primPath = Sdf.Path("../C.foo:bar:baz").GetPrimPath()
        self.assertEqual(primPath, Sdf.Path("../C"))
        
        primPath = Sdf.Path("../.foo[target].bar").GetPrimPath()
        self.assertEqual(primPath, Sdf.Path(".."))
        primPath = Sdf.Path("../.foo[target].bar:baz").GetPrimPath()
        self.assertEqual(primPath, Sdf.Path(".."))
        
        print '\tPassed'
        
        # ========================================================================
        # Test HasPrefix and ReplacePrefix
        # ========================================================================
        print "Test hasPrefix and replacePrefix"
        
        # Test HasPrefix
        self.assertFalse( Sdf.Path.emptyPath.HasPrefix('A') )
        self.assertFalse( Sdf.Path('A').HasPrefix( Sdf.Path.emptyPath ) )
        aPath = Sdf.Path("/Chars/Buzz_1/LArm.FB")
        self.assertEqual( aPath.HasPrefix( "/Chars/Buzz_1" ), True )
        self.assertEqual( aPath.HasPrefix( "Buzz_1" ), False )
        
        # Replace aPath's prefix and get a new path
        bPath = aPath.ReplacePrefix( "/Chars/Buzz_1", "/Chars/Buzz_2" )
        self.assertEqual( bPath, Sdf.Path("/Chars/Buzz_2/LArm.FB") )
        
        # Specify a bogus prefix to replace and get an empty path
        cPath = bPath.ReplacePrefix("/BadPrefix/Buzz_2", "/ReleasedChars/Buzz_2")
        self.assertEqual( cPath, bPath )
        
        # This formerly crashed due to a reference counting bug.
        p = Sdf.Path('/A/B.a[/C/D.a[/E/F.a]].a')
        p.ReplacePrefix('/E/F', '/X')
        p.ReplacePrefix('/E/F', '/X')
        p.ReplacePrefix('/E/F', '/X')
        p.ReplacePrefix('/E/F', '/X')
        
        # This formerly failed to replace due to an early out if the element count
        # was not longer than the element count of the prefix path we were replacing.
        p = Sdf.Path('/A.a[/B/C/D/E]') # Element count 3 [A, a, [/B/C/D/E] ]
        result = p.ReplacePrefix('/B/C/D/E', # Element count 4
                                 '/B/C/D/F')
        self.assertEqual(result, Sdf.Path('/A.a[/B/C/D/F]'))
        
        # Test replacing target paths.
        p = Sdf.Path('/A/B.a[/C/D.a[/A/F.a]].a')
        assert (p.ReplacePrefix('/A', '/_', fixTargetPaths=False)
                == Sdf.Path('/_/B.a[/C/D.a[/A/F.a]].a'))
        assert (p.ReplacePrefix('/A', '/_', fixTargetPaths=True)
                == Sdf.Path('/_/B.a[/C/D.a[/_/F.a]].a'))
        
        # ========================================================================
        # Test RemoveCommonSuffix
        # ========================================================================
        print "Test RemoveCommonSuffix"
        
        aPath = Sdf.Path('/A/B/C')
        bPath = Sdf.Path('/X/Y/Z')
        (r1, r2) = aPath.RemoveCommonSuffix(bPath)
        self.assertEqual(r1, Sdf.Path('/A/B/C'))
        self.assertEqual(r2, Sdf.Path('/X/Y/Z'))
        (r1, r2) = aPath.RemoveCommonSuffix(bPath, stopAtRootPrim=True)
        self.assertEqual(r1, Sdf.Path('/A/B/C'))
        self.assertEqual(r2, Sdf.Path('/X/Y/Z'))
        
        aPath = Sdf.Path('A/B/C')
        bPath = Sdf.Path('X/Y/Z')
        (r1, r2) = aPath.RemoveCommonSuffix(bPath)
        self.assertEqual(r1, Sdf.Path('A/B/C'))
        self.assertEqual(r2, Sdf.Path('X/Y/Z'))
        (r1, r2) = aPath.RemoveCommonSuffix(bPath, stopAtRootPrim=True)
        self.assertEqual(r1, Sdf.Path('A/B/C'))
        self.assertEqual(r2, Sdf.Path('X/Y/Z'))
        
        aPath = Sdf.Path('/A/B/C')
        bPath = Sdf.Path('/X/Y/C')
        (r1, r2) = aPath.RemoveCommonSuffix(bPath)
        self.assertEqual(r1, Sdf.Path('/A/B'))
        self.assertEqual(r2, Sdf.Path('/X/Y'))
        (r1, r2) = aPath.RemoveCommonSuffix(bPath, stopAtRootPrim=True)
        self.assertEqual(r1, Sdf.Path('/A/B'))
        self.assertEqual(r2, Sdf.Path('/X/Y'))
        
        aPath = Sdf.Path('A/B/C')
        bPath = Sdf.Path('X/Y/C')
        (r1, r2) = aPath.RemoveCommonSuffix(bPath)
        self.assertEqual(r1, Sdf.Path('A/B'))
        self.assertEqual(r2, Sdf.Path('X/Y'))
        (r1, r2) = aPath.RemoveCommonSuffix(bPath, stopAtRootPrim=True)
        self.assertEqual(r1, Sdf.Path('A/B'))
        self.assertEqual(r2, Sdf.Path('X/Y'))
        
        aPath = Sdf.Path('A/B/C')
        bPath = Sdf.Path('/X/Y/C')
        (r1, r2) = aPath.RemoveCommonSuffix(bPath)
        self.assertEqual(r1, Sdf.Path('A/B'))
        self.assertEqual(r2, Sdf.Path('/X/Y'))
        (r1, r2) = aPath.RemoveCommonSuffix(bPath, stopAtRootPrim=True)
        self.assertEqual(r1, Sdf.Path('A/B'))
        self.assertEqual(r2, Sdf.Path('/X/Y'))
        
        aPath = Sdf.Path('/A/B/C')
        bPath = Sdf.Path('/X/B/C')
        (r1, r2) = aPath.RemoveCommonSuffix(bPath)
        self.assertEqual(r1, Sdf.Path('/A'))
        self.assertEqual(r2, Sdf.Path('/X'))
        (r1, r2) = aPath.RemoveCommonSuffix(bPath, stopAtRootPrim=True)
        self.assertEqual(r1, Sdf.Path('/A'))
        self.assertEqual(r2, Sdf.Path('/X'))
        
        aPath = Sdf.Path('A/B/C')
        bPath = Sdf.Path('X/B/C')
        (r1, r2) = aPath.RemoveCommonSuffix(bPath)
        self.assertEqual(r1, Sdf.Path('A'))
        self.assertEqual(r2, Sdf.Path('X'))
        (r1, r2) = aPath.RemoveCommonSuffix(bPath, stopAtRootPrim=True)
        self.assertEqual(r1, Sdf.Path('A'))
        self.assertEqual(r2, Sdf.Path('X'))
        
        aPath = Sdf.Path('A/B/C')
        bPath = Sdf.Path('/X/B/C')
        (r1, r2) = aPath.RemoveCommonSuffix(bPath)
        self.assertEqual(r1, Sdf.Path('A'))
        self.assertEqual(r2, Sdf.Path('/X'))
        (r1, r2) = aPath.RemoveCommonSuffix(bPath, stopAtRootPrim=True)
        self.assertEqual(r1, Sdf.Path('A'))
        self.assertEqual(r2, Sdf.Path('/X'))
        
        aPath = Sdf.Path('/A/B/C')
        bPath = Sdf.Path('/A/B/C')
        (r1, r2) = aPath.RemoveCommonSuffix(bPath)
        self.assertEqual(r1, Sdf.Path('/'))
        self.assertEqual(r2, Sdf.Path('/'))
        (r1, r2) = aPath.RemoveCommonSuffix(bPath, stopAtRootPrim=True)
        self.assertEqual(r1, Sdf.Path('/A'))
        self.assertEqual(r2, Sdf.Path('/A'))
        
        aPath = Sdf.Path('A/B/C')
        bPath = Sdf.Path('A/B/C')
        (r1, r2) = aPath.RemoveCommonSuffix(bPath)
        self.assertEqual(r1, Sdf.Path('.'))
        self.assertEqual(r2, Sdf.Path('.'))
        (r1, r2) = aPath.RemoveCommonSuffix(bPath, stopAtRootPrim=True)
        self.assertEqual(r1, Sdf.Path('A'))
        self.assertEqual(r2, Sdf.Path('A'))
        
        aPath = Sdf.Path('A/B/C')
        bPath = Sdf.Path('/A/B/C')
        (r1, r2) = aPath.RemoveCommonSuffix(bPath)
        self.assertEqual(r1, Sdf.Path('.'))
        self.assertEqual(r2, Sdf.Path('/'))
        (r1, r2) = aPath.RemoveCommonSuffix(bPath, stopAtRootPrim=True)
        self.assertEqual(r1, Sdf.Path('A'))
        self.assertEqual(r2, Sdf.Path('/A'))
        
        aPath = Sdf.Path('/A/B/C')
        bPath = Sdf.Path('/X/C')
        (r1, r2) = aPath.RemoveCommonSuffix(bPath)
        self.assertEqual(r1, Sdf.Path('/A/B'))
        self.assertEqual(r2, Sdf.Path('/X'))
        (r1, r2) = aPath.RemoveCommonSuffix(bPath, stopAtRootPrim=True)
        self.assertEqual(r1, Sdf.Path('/A/B'))
        self.assertEqual(r2, Sdf.Path('/X'))
        
        aPath = Sdf.Path('A/B/C')
        bPath = Sdf.Path('X/C')
        (r1, r2) = aPath.RemoveCommonSuffix(bPath)
        self.assertEqual(r1, Sdf.Path('A/B'))
        self.assertEqual(r2, Sdf.Path('X'))
        (r1, r2) = aPath.RemoveCommonSuffix(bPath, stopAtRootPrim=True)
        self.assertEqual(r1, Sdf.Path('A/B'))
        self.assertEqual(r2, Sdf.Path('X'))
        
        aPath = Sdf.Path('/A/B/C')
        bPath = Sdf.Path('/B/C')
        (r1, r2) = aPath.RemoveCommonSuffix(bPath)
        self.assertEqual(r1, Sdf.Path('/A'))
        self.assertEqual(r2, Sdf.Path('/'))
        (r1, r2) = aPath.RemoveCommonSuffix(bPath, stopAtRootPrim=True)
        self.assertEqual(r1, Sdf.Path('/A/B'))
        self.assertEqual(r2, Sdf.Path('/B'))
        
        aPath = Sdf.Path('A/B/C')
        bPath = Sdf.Path('B/C')
        (r1, r2) = aPath.RemoveCommonSuffix(bPath)
        self.assertEqual(r1, Sdf.Path('A'))
        self.assertEqual(r2, Sdf.Path('.'))
        (r1, r2) = aPath.RemoveCommonSuffix(bPath, stopAtRootPrim=True)
        self.assertEqual(r1, Sdf.Path('A/B'))
        self.assertEqual(r2, Sdf.Path('B'))
        
        aPath = Sdf.Path('/A/B/C')
        bPath = Sdf.Path('/C')
        (r1, r2) = aPath.RemoveCommonSuffix(bPath)
        self.assertEqual(r1, Sdf.Path('/A/B'))
        self.assertEqual(r2, Sdf.Path('/'))
        (r1, r2) = aPath.RemoveCommonSuffix(bPath, stopAtRootPrim=True)
        self.assertEqual(r1, Sdf.Path('/A/B/C'))
        self.assertEqual(r2, Sdf.Path('/C'))
        
        aPath = Sdf.Path('A/B/C')
        bPath = Sdf.Path('C')
        (r1, r2) = aPath.RemoveCommonSuffix(bPath)
        self.assertEqual(r1, Sdf.Path('A/B'))
        self.assertEqual(r2, Sdf.Path('.'))
        (r1, r2) = aPath.RemoveCommonSuffix(bPath, stopAtRootPrim=True)
        self.assertEqual(r1, Sdf.Path('A/B/C'))
        self.assertEqual(r2, Sdf.Path('C'))
        
        aPath = Sdf.Path('A/B/C')
        bPath = Sdf.Path('/C')
        (r1, r2) = aPath.RemoveCommonSuffix(bPath)
        self.assertEqual(r1, Sdf.Path('A/B'))
        self.assertEqual(r2, Sdf.Path('/'))
        (r1, r2) = aPath.RemoveCommonSuffix(bPath, stopAtRootPrim=True)
        self.assertEqual(r1, Sdf.Path('A/B/C'))
        self.assertEqual(r2, Sdf.Path('/C'))
        
        aPath = Sdf.Path('/Lights/Lkey.shinesOn[/Chars/Buzz/Helmet].intensity')
        bPath = Sdf.Path('/Lights/Lkey.shinesOn[/Chars/Buzz].intensity')
        (r1, r2) = aPath.RemoveCommonSuffix(bPath)
        self.assertEqual(r1, Sdf.Path('/Lights/Lkey.shinesOn[/Chars/Buzz/Helmet]'))
        self.assertEqual(r2, Sdf.Path('/Lights/Lkey.shinesOn[/Chars/Buzz]'))
        (r1, r2) = aPath.RemoveCommonSuffix(bPath, stopAtRootPrim=True)
        self.assertEqual(r1, Sdf.Path('/Lights/Lkey.shinesOn[/Chars/Buzz/Helmet]'))
        self.assertEqual(r2, Sdf.Path('/Lights/Lkey.shinesOn[/Chars/Buzz]'))
        
        aPath = Sdf.Path('Lights/Lkey.shinesOn[/Chars/Buzz/Helmet].intensity')
        bPath = Sdf.Path('Lights/Lkey.shinesOn[/Chars/Buzz].intensity')
        (r1, r2) = aPath.RemoveCommonSuffix(bPath)
        self.assertEqual(r1, Sdf.Path('Lights/Lkey.shinesOn[/Chars/Buzz/Helmet]'))
        self.assertEqual(r2, Sdf.Path('Lights/Lkey.shinesOn[/Chars/Buzz]'))
        (r1, r2) = aPath.RemoveCommonSuffix(bPath, stopAtRootPrim=True)
        self.assertEqual(r1, Sdf.Path('Lights/Lkey.shinesOn[/Chars/Buzz/Helmet]'))
        self.assertEqual(r2, Sdf.Path('Lights/Lkey.shinesOn[/Chars/Buzz]'))
        
        aPath = Sdf.Path('/Lights/Lkey.shinesOn[/Chars/Buzz/Helmet].intensity')
        bPath = Sdf.Path('/Lights2/Lkey.shinesOn[/Chars/Buzz/Helmet].intensity')
        (r1, r2) = aPath.RemoveCommonSuffix(bPath)
        self.assertEqual(r1, Sdf.Path('/Lights'))
        self.assertEqual(r2, Sdf.Path('/Lights2'))
        (r1, r2) = aPath.RemoveCommonSuffix(bPath, stopAtRootPrim=True)
        self.assertEqual(r1, Sdf.Path('/Lights'))
        self.assertEqual(r2, Sdf.Path('/Lights2'))
        
        aPath = Sdf.Path('Lights/Lkey.shinesOn[/Chars/Buzz/Helmet].intensity')
        bPath = Sdf.Path('Lights2/Lkey.shinesOn[/Chars/Buzz/Helmet].intensity')
        (r1, r2) = aPath.RemoveCommonSuffix(bPath)
        self.assertEqual(r1, Sdf.Path('Lights'))
        self.assertEqual(r2, Sdf.Path('Lights2'))
        (r1, r2) = aPath.RemoveCommonSuffix(bPath, stopAtRootPrim=True)
        self.assertEqual(r1, Sdf.Path('Lights'))
        self.assertEqual(r2, Sdf.Path('Lights2'))
        
        # ========================================================================
        # Test GetTargetPath
        # ========================================================================
        print "Test targetPath"
        
        aPath = Sdf.Path("/Lights/Lkey.shinesOn[/Chars/Buzz/Helmet].intensity")
        self.assertEqual( aPath.targetPath, Sdf.Path("/Chars/Buzz/Helmet") )
        
        aPath = Sdf.Path("/Lights/Lkey.shinesOn[../../Buzz/Helmet].intensity")
        self.assertEqual( aPath.targetPath, Sdf.Path("../../Buzz/Helmet") )
        
        aPath = Sdf.Path("/Lights/Lkey.shinesOn[/Chars/Buzz/Helmet].intensity")
        self.assertEqual( aPath.targetPath, Sdf.Path("/Chars/Buzz/Helmet") )
        
        aPath = Sdf.Path("/Lights/Lkey.shinesOn[/Chars/Buzz/Helmet.blah].intensity")
        self.assertEqual( aPath.targetPath, Sdf.Path("/Chars/Buzz/Helmet.blah") )
        
        # No target
        aPath = Sdf.Path.emptyPath
        self.assertEqual( aPath.targetPath, Sdf.Path.emptyPath )
        
        
        # ========================================================================
        # Test GetAllTargetPathsRecursively
        # ========================================================================
        print "Test GetAllTargetPathsRecursively"
        
        aPath = Sdf.Path("/Lights/Lkey.shinesOn[/Chars/Buzz/Helmet].intensity")
        self.assertEqual( aPath.GetAllTargetPathsRecursively(), 
                     [Sdf.Path("/Chars/Buzz/Helmet")] )
        
        aPath = Sdf.Path("/Lights/Lkey.shinesOn[../../Buzz/Helmet].intensity")
        self.assertEqual( aPath.GetAllTargetPathsRecursively(),
                     [Sdf.Path("../../Buzz/Helmet")] )
        
        aPath = Sdf.Path("/Lights/Lkey.shinesOn[/Chars/Buzz/Helmet].intensity")
        self.assertEqual( aPath.GetAllTargetPathsRecursively(),
                     [Sdf.Path("/Chars/Buzz/Helmet")] )
        
        aPath = Sdf.Path("/Lights/Lkey.shinesOn[/Chars/Buzz/Helmet.blah].intensity")
        self.assertEqual( aPath.GetAllTargetPathsRecursively(),
                     [Sdf.Path("/Chars/Buzz/Helmet.blah")] )
        
        aPath = Sdf.Path('/A/B.a[/C/D.a[/E/F.a]].a')
        self.assertEqual(aPath.GetAllTargetPathsRecursively(),
                    [Sdf.Path(x) for x in ['/C/D.a[/E/F.a]', '/E/F.a']])
        
        aPath = Sdf.Path('/A/B.a[/C/D.a[/E/F.a]].a[/A/B.a[/C/D.a]]')
        self.assertEqual(aPath.GetAllTargetPathsRecursively(),
                    [Sdf.Path(x) for x in ['/A/B.a[/C/D.a]', '/C/D.a', 
                                          '/C/D.a[/E/F.a]', '/E/F.a']])
        
        aPath = Sdf.Path('/No/Target/Paths')
        self.assertEqual(aPath.GetAllTargetPathsRecursively(), [])
        
        
        # =======================================================================
        # Test AppendChild
        # =======================================================================
        print "Test appendChild"
        
        aPath = Sdf.Path("/foo")
        self.assertEqual( aPath.AppendChild("bar"), Sdf.Path("/foo/bar") )
        aPath = Sdf.Path("foo")
        self.assertEqual( aPath.AppendChild("bar"), Sdf.Path("foo/bar") )
        
        aPath = Sdf.Path("/foo.prop")
        self.assertEqual( aPath.AppendChild("bar"), Sdf.Path.emptyPath )
        
        # =======================================================================
        # Test AppendProperty
        # =======================================================================
        print "Test appendProperty"
        
        aPath = Sdf.Path("/foo")
        self.assertEqual( aPath.AppendProperty("prop"), Sdf.Path("/foo.prop") )
        self.assertEqual( aPath.AppendProperty("prop:foo:bar"), Sdf.Path("/foo.prop:foo:bar") )
        
        aPath = Sdf.Path("/foo.prop")
        self.assertEqual( aPath.AppendProperty("prop2"), Sdf.Path.emptyPath )
        self.assertEqual( aPath.AppendProperty("prop2:foo:bar"), Sdf.Path.emptyPath )
        
        # =======================================================================
        # Test AppendPath
        # =======================================================================
        print "Test AppendPath"
        
        # append to empty path -> empty path
        with self.assertRaises(Tf.ErrorException):
            Sdf.Path().AppendPath( Sdf.Path() )

        with self.assertRaises(Tf.ErrorException):
            Sdf.Path().AppendPath( Sdf.Path('A') )
        
        # append to root/prim path
        assert Sdf.Path('/').AppendPath( Sdf.Path('A') ) == Sdf.Path('/A')
        assert Sdf.Path('/A').AppendPath( Sdf.Path('B') ) == Sdf.Path('/A/B')
        
        # append empty to root/prim path -> no change
        with self.assertRaises(Tf.ErrorException):
            Sdf.Path('/').AppendPath( Sdf.Path() )

        with self.assertRaises(Tf.ErrorException):
            Sdf.Path('/A').AppendPath( Sdf.Path() )
        
        # =======================================================================
        # Test AppendTarget
        # =======================================================================
        print "Test appendTarget"
        
        aPath = Sdf.Path("/foo.rel")
        self.assertEqual( aPath.AppendTarget("/Bar/Baz"), Sdf.Path("/foo.rel[/Bar/Baz]") )
        
        aPath = Sdf.Path("/foo")
        self.assertEqual( aPath.AppendTarget("/Bar/Baz"), Sdf.Path.emptyPath )
        
        # =======================================================================
        # Test AppendRelationalAttribute
        # =======================================================================
        print "Test appendRelationalAttribute"
        
        aPath = Sdf.Path("/foo.rel[/Bar/Baz]")
        self.assertEqual( aPath.AppendRelationalAttribute("attr"), Sdf.Path("/foo.rel[/Bar/Baz].attr") )
        self.assertEqual( aPath.AppendRelationalAttribute("attr:foo:bar"), Sdf.Path("/foo.rel[/Bar/Baz].attr:foo:bar") )
        
        aPath = Sdf.Path("/foo")
        self.assertEqual( aPath.AppendRelationalAttribute("attr"), Sdf.Path.emptyPath )
        self.assertEqual( aPath.AppendRelationalAttribute("attr:foo:bar"), Sdf.Path.emptyPath )
        
        # =======================================================================
        # Test GetParentPath and GetName
        # =======================================================================
        print "Test parentPath, name, and replaceName"
        
        self.assertEqual(Sdf.Path("/foo/bar/baz").GetParentPath(), Sdf.Path("/foo/bar"))
        self.assertEqual(Sdf.Path("/foo").GetParentPath(), Sdf.Path("/"))
        self.assertEqual(Sdf.Path.emptyPath.GetParentPath(), Sdf.Path.emptyPath)
        self.assertEqual(Sdf.Path("/foo/bar/baz.prop").GetParentPath(), Sdf.Path("/foo/bar/baz"))
        self.assertEqual(Sdf.Path("/foo/bar/baz.rel[/targ].attr").GetParentPath(), Sdf.Path("/foo/bar/baz.rel[/targ]"))
        self.assertEqual(Sdf.Path("/foo/bar/baz.rel[/targ]").GetParentPath(), Sdf.Path("/foo/bar/baz.rel"))
        self.assertEqual(Sdf.Path("../../..").GetParentPath(), Sdf.Path("../../../.."))
        self.assertEqual(Sdf.Path("..").GetParentPath(), Sdf.Path("../.."))
        self.assertEqual(Sdf.Path("../../../.prop").GetParentPath(), Sdf.Path("../../.."))
        self.assertEqual(Sdf.Path("../../../.rel[/targ]").GetParentPath(), Sdf.Path("../../../.rel"))
        self.assertEqual(Sdf.Path("../../../.rel[/targ].attr").GetParentPath(), Sdf.Path("../../../.rel[/targ]"))
        self.assertEqual(Sdf.Path("foo/bar/baz").GetParentPath(), Sdf.Path("foo/bar"))
        self.assertEqual(Sdf.Path("foo").GetParentPath(), Sdf.Path("."))
        self.assertEqual(Sdf.Path("foo/bar/baz.prop").GetParentPath(), Sdf.Path("foo/bar/baz"))
        self.assertEqual(Sdf.Path("foo/bar/baz.rel[/targ]").GetParentPath(), Sdf.Path("foo/bar/baz.rel"))
        self.assertEqual(Sdf.Path("foo/bar/baz.rel[/targ].attr").GetParentPath(), Sdf.Path("foo/bar/baz.rel[/targ]"))
        
        self.assertEqual(Sdf.Path("/foo/bar/baz").name, "baz")
        self.assertEqual(Sdf.Path("/foo").name, "foo")
        self.assertEqual(Sdf.Path.emptyPath.name, "")
        self.assertEqual(Sdf.Path("/foo/bar/baz.prop").name, "prop")
        self.assertEqual(Sdf.Path("/foo/bar/baz.prop:argle:bargle").name, "prop:argle:bargle")
        self.assertEqual(Sdf.Path("/foo/bar/baz.rel[/targ].attr").name, "attr")
        self.assertEqual(Sdf.Path("/foo/bar/baz.rel[/targ].attr:argle:bargle").name, "attr:argle:bargle")
        self.assertEqual(Sdf.Path("../../..").name, "..")
        self.assertEqual(Sdf.Path("../../.prop").name, "prop")
        self.assertEqual(Sdf.Path("../../.prop:argle:bargle").name, "prop:argle:bargle")
        self.assertEqual(Sdf.Path("../../.rel[/targ].attr").name, "attr")
        self.assertEqual(Sdf.Path("../../.rel[/targ].attr:argle:bargle").name, "attr:argle:bargle")
        self.assertEqual(Sdf.Path("foo/bar/baz").name, "baz")
        self.assertEqual(Sdf.Path("foo").name, "foo")
        self.assertEqual(Sdf.Path("foo/bar/baz.prop").name, "prop")
        self.assertEqual(Sdf.Path("foo/bar/baz.prop:argle:bargle").name, "prop:argle:bargle")
        self.assertEqual(Sdf.Path("foo/bar/baz.rel[/targ].attr").name, "attr")
        self.assertEqual(Sdf.Path("foo/bar/baz.rel[/targ].attr:argle:bargle").name, "attr:argle:bargle")
        
        self.assertEqual(Sdf.Path("/foo/bar/baz").ReplaceName('foo'), Sdf.Path("/foo/bar/foo"))
        self.assertEqual(Sdf.Path("/foo").ReplaceName('bar'), Sdf.Path("/bar"))
        self.assertEqual(Sdf.Path("/foo/bar/baz.prop").ReplaceName('attr'),
                    Sdf.Path("/foo/bar/baz.attr"))
        self.assertEqual(Sdf.Path("/foo/bar/baz.prop").ReplaceName('attr:argle:bargle'),
                    Sdf.Path("/foo/bar/baz.attr:argle:bargle"))
        self.assertEqual(Sdf.Path("/foo/bar/baz.prop:argle:bargle").ReplaceName('attr'),
                    Sdf.Path("/foo/bar/baz.attr"))
        self.assertEqual(Sdf.Path("/foo/bar/baz.prop:argle:bargle").ReplaceName('attr:foo:fa:raw'),
                    Sdf.Path("/foo/bar/baz.attr:foo:fa:raw"))
        self.assertEqual(Sdf.Path("/foo/bar/baz.rel[/targ].attr").ReplaceName('prop'),
                    Sdf.Path("/foo/bar/baz.rel[/targ].prop"))
        self.assertEqual(Sdf.Path("/foo/bar/baz.rel[/targ].attr").ReplaceName('prop:argle:bargle'),
                    Sdf.Path("/foo/bar/baz.rel[/targ].prop:argle:bargle"))
        self.assertEqual(Sdf.Path("/foo/bar/baz.rel[/targ].attr:argle:bargle").ReplaceName('prop'),
                    Sdf.Path("/foo/bar/baz.rel[/targ].prop"))
        self.assertEqual(Sdf.Path("/foo/bar/baz.rel[/targ].attr:argle:bargle").ReplaceName('prop:foo:fa:raw'),
                    Sdf.Path("/foo/bar/baz.rel[/targ].prop:foo:fa:raw"))
        self.assertEqual(Sdf.Path("../../..").ReplaceName('foo'), Sdf.Path("../../../../foo"))
        self.assertEqual(Sdf.Path("..").ReplaceName('foo'), Sdf.Path("../../foo"))
        self.assertEqual(Sdf.Path("../../../.prop").ReplaceName('attr'),
                    Sdf.Path("../../../.attr"))
        self.assertEqual(Sdf.Path("../../../.prop").ReplaceName('attr:argle:bargle'),
                    Sdf.Path("../../../.attr:argle:bargle"))
        self.assertEqual(Sdf.Path("../../../.rel[/targ].attr").ReplaceName('prop'),
                    Sdf.Path("../../../.rel[/targ].prop"))
        self.assertEqual(Sdf.Path("../../../.rel[/targ].attr").ReplaceName('prop:argle:bargle'),
                    Sdf.Path("../../../.rel[/targ].prop:argle:bargle"))
        self.assertEqual(Sdf.Path("foo/bar/baz").ReplaceName('foo'), Sdf.Path("foo/bar/foo"))
        self.assertEqual(Sdf.Path("foo").ReplaceName('bar'), Sdf.Path("bar"))
        self.assertEqual(Sdf.Path("foo/bar/baz.prop").ReplaceName('attr'),
                    Sdf.Path("foo/bar/baz.attr"))
        self.assertEqual(Sdf.Path("foo/bar/baz.prop").ReplaceName('attr:argle:bargle'),
                    Sdf.Path("foo/bar/baz.attr:argle:bargle"))
        self.assertEqual(Sdf.Path("foo/bar/baz.rel[/targ].attr").ReplaceName('prop'), 
                    Sdf.Path("foo/bar/baz.rel[/targ].prop"))
        self.assertEqual(Sdf.Path("foo/bar/baz.rel[/targ].attr").ReplaceName('prop:argle:bargle'), 
                    Sdf.Path("foo/bar/baz.rel[/targ].prop:argle:bargle"))
        
        with self.assertRaises(Tf.ErrorException):
            Sdf.Path('/foo/bar[target]').ReplaceName('xxx')

        # =======================================================================
        # Test GetConciseRelativePaths
        # =======================================================================
        print "Test GetConciseRelativePaths"
        
        aPath = Sdf.Path("/foo/bar")
        bPath = Sdf.Path("/foo/baz")
        cPath = Sdf.Path("/foo")
        
        # a typical assortment of paths
        self.assertEqual( Sdf.Path.GetConciseRelativePaths([aPath,bPath,cPath]),
                       [Sdf.Path("bar"), Sdf.Path("baz"), Sdf.Path("/foo")] )
        
        # test some property paths
        dPath = Sdf.Path("/foo/bar.a")
        ePath = Sdf.Path("/foo/bar.b")
        
        self.assertEqual( Sdf.Path.GetConciseRelativePaths([dPath, ePath]),
                        [Sdf.Path("bar.a"), Sdf.Path("bar.b")] )
        
        fPath = Sdf.Path("/baz/bar")
        
        self.assertEqual( Sdf.Path.GetConciseRelativePaths([aPath, fPath]),
                     [Sdf.Path("/foo/bar"), Sdf.Path("/baz/bar")] )
        
        # now give it two identical paths
        self.assertEqual( Sdf.Path.GetConciseRelativePaths([aPath, aPath]),
                     [Sdf.Path("bar"), Sdf.Path("bar")] )
        
        # give it root paths
        gPath = Sdf.Path("/bar")
        self.assertEqual( Sdf.Path.GetConciseRelativePaths([cPath, gPath]),
                     [Sdf.Path("/foo"), Sdf.Path("/bar")] )
        
        # now give it a relative path as an argument
        self.assertEqual( Sdf.Path.GetConciseRelativePaths([Sdf.Path("a")]),
                     [Sdf.Path("a")] )
        
        
        # =======================================================================
        # Test RemoveDescendentPaths
        # =======================================================================
        
        print "Test RemoveDescendentPaths"
        
        paths = [Sdf.Path(x) for x in
                 ['/a/b/c', '/q', '/a/b/c/d/e/f/g', '/r/s/t', '/a/b', 
                  '/q/r/s/t', '/x/y', '/a/b/d']]
        
        expected = [Sdf.Path(x) for x in ['/a/b', '/q', '/x/y', '/r/s/t']]
        
        result = Sdf.Path.RemoveDescendentPaths(paths)
        
        # ensure result is unique, then compare independent of order.
        self.assertEqual(len(result), len(set(result)))
        self.assertEqual(set(result), set(expected))
        
        
        # =======================================================================
        # Test RemoveAncestorPaths
        # =======================================================================
        
        print "Test RemoveAncestorPaths"
        
        paths = [Sdf.Path(x) for x in
                 ['/a/b/c', '/q', '/a/b/c/d/e/f/g', '/r/s/t', '/a/b', 
                  '/q/r/s/t', '/x/y', '/a/b/d']]
        
        expected = [Sdf.Path(x) for x in 
                    ['/a/b/c/d/e/f/g', '/a/b/d', '/q/r/s/t', '/r/s/t', '/x/y']]
        
        result = Sdf.Path.RemoveAncestorPaths(paths)
        
        # ensure result is unique, then compare independent of order.
        self.assertEqual(len(result), len(set(result)))
        self.assertEqual(set(result), set(expected))
        
        # ========================================================================
        # Test FindPrefixedRange and FindLongestPrefix
        # ========================================================================
        
        def testFindPrefixedRangeAndFindLongestPrefix():
            print "Test FindPrefixedRange and FindLongestPrefix"
        
            import random, time
            rgen = random.Random()
            seed = int(time.time())
            rgen.seed(seed)
            print 'random seed', seed
        
            letters = [chr(x) for x in range(ord('a'), ord('d')+1)]
            maxLen = 8
        
            paths = []
            for i in range(300):
                elems = [rgen.choice(letters) for i in range(rgen.randint(1, maxLen))]
                paths.append(Sdf.Path('/' + '/'.join(elems)))
            paths.append(Sdf.Path('/'))
        
            tests = []
            for i in range(300):
                elems = [rgen.choice(letters) for i in range(rgen.randint(1, maxLen))]
                tests.append(Sdf.Path('/' + '/'.join(elems)))
            tests.append(Sdf.Path('/'))
        
            paths.sort()
            tests.sort()
        
            # print '== paths', '='*64
            # print '\n'.join(map(str, paths))
        
            # print '== tests', '='*64
            # print '\n'.join(map(str, tests))
        
            def testFindPrefixedRange(p, paths):
                sl = Sdf.Path.FindPrefixedRange(paths, p)
                #print p, '>>', ', '.join([str(x) for x in paths[sl]])
                self.assertTrue(all([path.HasPrefix(p) for path in paths[sl]]))
                others = list(paths)
                del others[sl]
                self.assertTrue(not any([path.HasPrefix(p) for path in others[sl]]))
        
            def testFindLongestPrefix(p, paths):
                lp = Sdf.Path.FindLongestPrefix(paths, p)
                # should always have some prefix since '/' is in paths.
                self.assertTrue(p.HasPrefix(lp))
                # manually find longest prefix
                bruteLongest = Sdf.Path('/')
                for x in paths:
                    if (p.HasPrefix(x) and 
                        x.pathElementCount > bruteLongest.pathElementCount):
                        bruteLongest = x
                # bruteLongest should match.
                #print 'path:', p, 'lp:', lp, 'bruteLongest:', bruteLongest
                self.assertEqual(lp, bruteLongest, ('lp (%s) != bruteLongest (%s)' % 
                                            (lp, bruteLongest)))
        
            for testp in tests:
                testFindPrefixedRange(testp, paths)
                testFindLongestPrefix(testp, paths)
        
            # Do a few simple cases directly.
            paths = map(Sdf.Path, ['/a', '/a/b/c/d', '/b/a', '/b/c/d/e'])
            flp = Sdf.Path.FindLongestPrefix
            assert flp(paths, '/x') == None
            assert flp(paths, '/a') == Sdf.Path('/a')
            assert flp(paths, '/a/a/a') == Sdf.Path('/a')
            assert flp(paths, '/a/c/d/e/f') == Sdf.Path('/a')
            assert flp(paths, '/a/b/c/d') == Sdf.Path('/a/b/c/d')
            assert flp(paths, '/a/b/c/d/a/b/c/d') == Sdf.Path('/a/b/c/d')
            assert flp(paths, '/a/b/c/e/f/g') == Sdf.Path('/a')
            assert flp(paths, '/b') == None
            assert flp(paths, '/b/a/b/c/d/e') == Sdf.Path('/b/a')
            assert flp(paths, '/b/c/d/e/f/g') == Sdf.Path('/b/c/d/e')
            assert flp(paths, '/b/c/d/e') == Sdf.Path('/b/c/d/e')
            assert flp(paths, '/b/c/x/y/z') == None
        
        testFindPrefixedRangeAndFindLongestPrefix()
        
        Sdf._DumpPathStats()
        
        print '\tPassed'
        
        print 'Test SUCCEEDED'

if __name__ == "__main__":
    unittest.main()
