#!/pxrpythonsubst
#
# Copyright 2024 Pixar
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

from pxr import Sdf, Tf
import sys, unittest

MatchEval = Sdf._MakeBasicMatchEval

class TestSdfPathExpression(unittest.TestCase):

    def test_Basics(self):
        # Empty expr.
        pe = Sdf.PathExpression()
        self.assertTrue(pe.IsEmpty())
        self.assertEqual(pe, Sdf.PathExpression.Nothing())
        self.assertEqual(Sdf.PathExpression.MakeComplement(pe),
                         Sdf.PathExpression.Everything())
        self.assertEqual(
            pe, Sdf.PathExpression.MakeComplement(
                Sdf.PathExpression.Everything()))
        self.assertEqual(pe, Sdf.PathExpression(''))
        self.assertFalse(pe)

        # Complement of complement should cancel.
        self.assertEqual(
            Sdf.PathExpression('~(~a)'), Sdf.PathExpression('a'))
        self.assertEqual(
            Sdf.PathExpression('~(~(~a))'), Sdf.PathExpression('~a'))
        self.assertEqual(
            Sdf.PathExpression('~(~(~(~a)))'), Sdf.PathExpression('a'))
        self.assertEqual(
            Sdf.PathExpression('// - a'), Sdf.PathExpression('~a'))
        self.assertEqual(
            Sdf.PathExpression('~(// - a)'), Sdf.PathExpression('a'))
        self.assertEqual(
            Sdf.PathExpression('~(// - ~a)'), Sdf.PathExpression('~a'))

    def test_Matching(self):

        evl = MatchEval('/foo/bar/*') 
        self.assertFalse(evl.Match('/foo'))
        self.assertFalse(evl.Match('/foo/bar'))
        self.assertTrue(evl.Match('/foo/bar/a'))
        self.assertTrue(evl.Match('/foo/bar/b'))
        self.assertTrue(evl.Match('/foo/bar/c'))
        self.assertFalse(evl.Match('/foo/bar/a/x'))
        self.assertFalse(evl.Match('/foo/bar/a/y'))
        self.assertFalse(evl.Match('/foo/bar/a/z'))
        
        evl = MatchEval('/foo//bar')
        self.assertTrue(evl.Match(Sdf.Path("/foo/bar")))
        self.assertTrue(evl.Match(Sdf.Path("/foo/x/bar")))
        self.assertTrue(evl.Match(Sdf.Path("/foo/x/y/z/bar")))
        self.assertFalse(evl.Match(Sdf.Path("/foo/x/y/z/bar/baz")))
        self.assertFalse(evl.Match(Sdf.Path("/foo/x/y/z/bar.baz")))
        self.assertFalse(evl.Match(Sdf.Path("/foo/x/y/z/bar.baz:buz")))

        evl = MatchEval("//foo/bar/baz/qux/quux")

        self.assertFalse(evl.Match(Sdf.Path("/foo")))
        self.assertFalse(evl.Match(Sdf.Path("/foo/bar")))
        self.assertFalse(evl.Match(Sdf.Path("/foo/bar/baz")))
        self.assertFalse(evl.Match(Sdf.Path("/foo/bar/baz/qux")))

        self.assertTrue(evl.Match(Sdf.Path("/foo/bar/baz/qux/quux")))
        self.assertTrue(evl.Match(Sdf.Path("/foo/foo/bar/baz/qux/quux")))
        self.assertTrue(evl.Match(Sdf.Path("/foo/bar/foo/bar/baz/qux/quux")))
        self.assertTrue(evl.Match(Sdf.Path("/foo/bar/baz/foo/bar/baz/qux/quux")))
        self.assertTrue(evl.Match(Sdf.Path("/foo/bar/baz/qux/foo/bar/baz/qux/quux")))

        evl = MatchEval("/foo*//bar")
        
        self.assertTrue(evl.Match(Sdf.Path("/foo/bar")))
        self.assertTrue(evl.Match(Sdf.Path("/foo/x/bar")))
        self.assertTrue(evl.Match(Sdf.Path("/foo/x/y/z/bar")))
        self.assertFalse(evl.Match(Sdf.Path("/foo/x/y/z/bar/baz")))
        self.assertFalse(evl.Match(Sdf.Path("/foo/x/y/z/bar.baz")))
        self.assertFalse(evl.Match(Sdf.Path("/foo/x/y/z/bar.baz:buz")))

        self.assertTrue(evl.Match(Sdf.Path("/foo1/bar")))
        self.assertTrue(evl.Match(Sdf.Path("/foo12/x/bar")))
        self.assertTrue(evl.Match(Sdf.Path("/fooBar/x/y/z/bar")))
        self.assertFalse(evl.Match(Sdf.Path("/fooX/x/y/z/bar/baz")))
        self.assertFalse(evl.Match(Sdf.Path("/fooY/x/y/z/bar.baz")))
        self.assertFalse(evl.Match(Sdf.Path("/fooY/x/y/z/bar.baz:buz")))

        evl = MatchEval("/foo*//bar{isPrimPath}")
        
        self.assertTrue(evl.Match(Sdf.Path("/foo/bar")))
        self.assertTrue(evl.Match(Sdf.Path("/foo/x/bar")))
        self.assertTrue(evl.Match(Sdf.Path("/foo/x/y/z/bar")))
        self.assertFalse(evl.Match(Sdf.Path("/foo/x/y/z/bar/baz")))
        self.assertFalse(evl.Match(Sdf.Path("/foo/x/y/z/bar.baz")))
        self.assertFalse(evl.Match(Sdf.Path("/foo/x/y/z/bar.baz:buz")))

        self.assertTrue(evl.Match(Sdf.Path("/foo1/bar")))
        self.assertTrue(evl.Match(Sdf.Path("/foo12/x/bar")))
        self.assertTrue(evl.Match(Sdf.Path("/fooBar/x/y/z/bar")))
        self.assertFalse(evl.Match(Sdf.Path("/fooX/x/y/z/bar/baz")))
        self.assertFalse(evl.Match(Sdf.Path("/fooY/x/y/z/bar.baz")))
        self.assertFalse(evl.Match(Sdf.Path("/fooY/x/y/z/bar.baz:buz")))

        evl = MatchEval("/foo*//bar//{isPrimPath}")
        
        self.assertTrue(evl.Match(Sdf.Path("/foo/bar/a")))
        self.assertTrue(evl.Match(Sdf.Path("/foo/x/bar/b")))
        self.assertTrue(evl.Match(Sdf.Path("/foo/x/y/z/bar/c")))
        self.assertTrue(evl.Match(Sdf.Path("/foo/x/y/z/bar/baz")))
        self.assertTrue(evl.Match(Sdf.Path("/foo/x/y/z/bar/baz/qux")))
        self.assertFalse(evl.Match(Sdf.Path("/foo/x/y/z/bar/baz.attr")))
        self.assertFalse(evl.Match(Sdf.Path("/foo/x/y/z/bar/baz/qux.attr")))
        self.assertFalse(evl.Match(Sdf.Path("/foo/x/y/z/bar/baz/qux.ns:attr")))

        self.assertTrue(evl.Match(Sdf.Path("/fooXYZ/bar/a")))
        self.assertTrue(evl.Match(Sdf.Path("/fooABC/x/bar/a/b/c")))
        self.assertTrue(evl.Match(Sdf.Path("/foo123/x/y/z/bar/x")))
        self.assertTrue(evl.Match(Sdf.Path("/fooASDF/x/y/z/bar/baz")))
        self.assertTrue(evl.Match(Sdf.Path("/foo___/x/y/z/bar/baz/qux")))
        self.assertFalse(evl.Match(Sdf.Path("/foo_bar/x/y/z/bar/baz.attr")))
        self.assertFalse(evl.Match(Sdf.Path("/foo_baz/x/y/z/bar/baz/qux.attr")))
        self.assertFalse(
            evl.Match(Sdf.Path("/foo_baz/x/y/z/bar/baz/qux.ns:attr")))

        evl = MatchEval("/a /b /c /d/e/f")

        self.assertTrue(evl.Match(Sdf.Path("/a")))
        self.assertTrue(evl.Match(Sdf.Path("/b")))
        self.assertTrue(evl.Match(Sdf.Path("/c")))
        self.assertTrue(evl.Match(Sdf.Path("/d/e/f")))

        self.assertFalse(evl.Match(Sdf.Path("/a/b")))
        self.assertFalse(evl.Match(Sdf.Path("/b/c")))
        self.assertFalse(evl.Match(Sdf.Path("/c/d")))
        self.assertFalse(evl.Match(Sdf.Path("/d/e")))

        evl = MatchEval("/a// - /a/b/c")

        self.assertTrue(evl.Match(Sdf.Path("/a")))
        self.assertTrue(evl.Match(Sdf.Path("/a/b")))
        self.assertFalse(evl.Match(Sdf.Path("/a/b/c")))
        self.assertTrue(evl.Match(Sdf.Path("/a/b/c/d")))
        self.assertTrue(evl.Match(Sdf.Path("/a/b/x")))
        self.assertTrue(evl.Match(Sdf.Path("/a/b/y")))

        evl = MatchEval("/a//{isPropertyPath} - /a/b.c")

        self.assertFalse(evl.Match(Sdf.Path("/a")))
        self.assertTrue(evl.Match(Sdf.Path("/a.b")))
        self.assertFalse(evl.Match(Sdf.Path("/a/b")))
        self.assertFalse(evl.Match(Sdf.Path("/a/b.c")))
        self.assertTrue(evl.Match(Sdf.Path("/a/b.ns:c")))
        self.assertTrue(evl.Match(Sdf.Path("/a/b.yes")))
        self.assertTrue(evl.Match(Sdf.Path("/a/b.ns:yes")))
        self.assertFalse(evl.Match(Sdf.Path("/a/b/c")))
        self.assertTrue(evl.Match(Sdf.Path("/a/b/c.d")))
        self.assertTrue(evl.Match(Sdf.Path("/a/b/c.ns:d")))
        self.assertFalse(evl.Match(Sdf.Path("/a/b/x")))
        self.assertTrue(evl.Match(Sdf.Path("/a/b/x.y")))
        self.assertTrue(evl.Match(Sdf.Path("/a/b/x.ns:y")))

    def test_ComposeOver(self):
        # ComposeOver
        a = Sdf.PathExpression("/a")
        b = Sdf.PathExpression("%_ /b")
        c = Sdf.PathExpression("%_ /c")

        self.assertFalse(a.ContainsExpressionReferences())
        self.assertFalse(a.ContainsWeakerExpressionReference())
        self.assertTrue(b.ContainsExpressionReferences())
        self.assertTrue(b.ContainsWeakerExpressionReference())
        self.assertTrue(c.ContainsExpressionReferences())
        self.assertTrue(c.ContainsWeakerExpressionReference())
        
        composed = c.ComposeOver(b).ComposeOver(a)

        self.assertFalse(composed.ContainsExpressionReferences())
        self.assertFalse(composed.ContainsWeakerExpressionReference())
        self.assertTrue(composed.IsComplete())
        
        evl = MatchEval(composed.GetText())

        self.assertTrue(evl.Match(Sdf.Path("/a")))
        self.assertTrue(evl.Match(Sdf.Path("/b")))
        self.assertTrue(evl.Match(Sdf.Path("/c")))
        self.assertFalse(evl.Match(Sdf.Path("/d")))

    def test_ResolveReferences(self):

        refs = Sdf.PathExpression("/a %_ %:foo - %:bar")
        weaker = Sdf.PathExpression("/weaker")
        foo = Sdf.PathExpression("/foo//")
        bar = Sdf.PathExpression("/foo/bar//")
        
        self.assertTrue(refs.ContainsExpressionReferences())
        self.assertFalse(weaker.ContainsExpressionReferences())
        self.assertFalse(foo.ContainsExpressionReferences())
        self.assertFalse(bar.ContainsExpressionReferences())

        def resolveRefs(ref):
            if ref.name == "_":
                return weaker
            elif ref.name == "foo":
                return foo
            elif ref.name == "bar":
                return bar
            else:
                return Sdf.PathExpression()

        resolved = refs.ResolveReferences(resolveRefs)

        self.assertFalse(resolved.ContainsExpressionReferences())
        self.assertTrue(resolved.IsComplete())

        # Resolved should be "/a /weaker /foo// - /foo/bar//"
        evl = MatchEval(resolved.GetText())

        self.assertTrue(evl.Match(Sdf.Path("/a")))
        self.assertTrue(evl.Match(Sdf.Path("/weaker")))
        self.assertTrue(evl.Match(Sdf.Path("/foo")))
        self.assertTrue(evl.Match(Sdf.Path("/foo/child")))
        self.assertFalse(evl.Match(Sdf.Path("/a/b")))
        self.assertFalse(evl.Match(Sdf.Path("/weaker/c")))
        self.assertFalse(evl.Match(Sdf.Path("/foo/bar")))
        self.assertFalse(evl.Match(Sdf.Path("/foo/bar/baz")))

        # ResolveReferences() with the empty expression should produce the empty
        # expression.
        self.assertTrue(
            Sdf.PathExpression().ResolveReferences(resolveRefs).IsEmpty())

    def test_MakeAbsolute(self):
        # Check MakeAbsolute.
        e = Sdf.PathExpression("foo ../bar baz//qux")
        self.assertFalse(e.IsAbsolute())
        self.assertFalse(e.ContainsExpressionReferences())
        abso = e.MakeAbsolute(Sdf.Path("/World/test"))
        # abso should be: "/World/test/foo /World/bar /World/test/baz//qux"
        self.assertTrue(abso.IsAbsolute())
        self.assertTrue(abso.IsComplete())

        evl = MatchEval(abso.GetText())

        self.assertTrue(evl.Match(Sdf.Path("/World/test/foo")))
        self.assertFalse(evl.Match(Sdf.Path("/World/test/bar")))
        self.assertTrue(evl.Match(Sdf.Path("/World/bar")))
        self.assertTrue(evl.Match(Sdf.Path("/World/test/baz/qux")))
        self.assertTrue(evl.Match(Sdf.Path("/World/test/baz/a/b/c/qux")))

    def test_ReplacePrefix(self):
        abso = Sdf.PathExpression(
            "/World/test/foo /World/bar /World/test/baz//qux")
        home = abso.ReplacePrefix(Sdf.Path("/World"), Sdf.Path("/Home"))
            
        evl = MatchEval(home.GetText())

        self.assertTrue(evl.Match(Sdf.Path("/Home/test/foo")))
        self.assertFalse(evl.Match(Sdf.Path("/Home/test/bar")))
        self.assertTrue(evl.Match(Sdf.Path("/Home/bar")))
        self.assertTrue(evl.Match(Sdf.Path("/Home/test/baz/qux")))
        self.assertTrue(evl.Match(Sdf.Path("/Home/test/baz/a/b/c/qux")))

    def test_PrefixConstancy(self):
        # Check constancy wrt prefix relations.
        evl = MatchEval("/prefix/path//")

        self.assertFalse(evl.Match(Sdf.Path("/prefix")))
        self.assertFalse(evl.Match(Sdf.Path("/prefix")).IsConstant())
        self.assertTrue(evl.Match(Sdf.Path("/prefix/path")))
        self.assertTrue(evl.Match(Sdf.Path("/prefix/path")).IsConstant())
        self.assertFalse(evl.Match(Sdf.Path("/prefix/wrong")))
        self.assertTrue(evl.Match(Sdf.Path("/prefix/wrong")).IsConstant())
        
        evl = MatchEval("//World//")
        self.assertTrue(evl.Match(Sdf.Path("/World")))
        self.assertTrue(evl.Match(Sdf.Path("/World")).IsConstant())
        self.assertTrue(evl.Match(Sdf.Path("/World/Foo")))
        self.assertTrue(evl.Match(Sdf.Path("/World/Foo")).IsConstant())

        evl = MatchEval("//World//Foo/Bar//")
        self.assertTrue(evl.Match(Sdf.Path("/World/Foo/Bar")))
        self.assertTrue(evl.Match(Sdf.Path("/World/Foo/Bar")).IsConstant())
        self.assertTrue(evl.Match(Sdf.Path("/World/X/Foo/Bar")))
        self.assertTrue(evl.Match(Sdf.Path("/World/X/Foo/Bar")).IsConstant())
        self.assertTrue(evl.Match(Sdf.Path("/World/X/Y/Foo/Bar")))
        self.assertTrue(evl.Match(Sdf.Path("/World/X/Y/Foo/Bar")).IsConstant())
        self.assertTrue(evl.Match(Sdf.Path("/World/X/Y/Foo/Bar/Baz")))
        self.assertTrue(evl.Match(Sdf.Path("/World/X/Y/Foo/Bar/Baz"))
                        .IsConstant())
        self.assertTrue(evl.Match(Sdf.Path("/World/X/Y/Foo/Bar/Baz/Qux")))
        self.assertTrue(evl.Match(Sdf.Path("/World/X/Y/Foo/Bar/Baz/Qux"))
                        .IsConstant())

    def test_SceneDescription(self):
        l = Sdf.Layer.CreateAnonymous()
        prim = Sdf.CreatePrimInLayer(l, "/prim")
        attr = Sdf.AttributeSpec(
            prim, "attr", Sdf.ValueTypeNames.PathExpression)
        self.assertTrue(attr)
        attr.default = Sdf.PathExpression("child")
        # Should have been made absolute:
        self.assertEqual(attr.default, Sdf.PathExpression("/prim/child"))

if __name__ == '__main__':
    unittest.main()
