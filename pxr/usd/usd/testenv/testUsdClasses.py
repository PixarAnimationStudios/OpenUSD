#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

from pxr import Sdf, Usd, Tf
import unittest

allFormats = ['usd' + x for x in 'ac']

class TestUsdClasses(unittest.TestCase):
    def test_Basic(self):
        for fmt in allFormats:
            l = Sdf.Layer.CreateAnonymous('BasicClassTest.'+fmt)

            stage = Usd.Stage.Open(l.identifier)

            # Create a new class "foo" and set some attributes.
            f = stage.CreateClassPrim("/foo")
            a =  f.CreateAttribute("bar", Sdf.ValueTypeNames.Int)
            a.Set(42)
            self.assertEqual(a.Get(), 42)

            a = f.CreateAttribute("baz", Sdf.ValueTypeNames.Int)
            a.Set(24)
            self.assertEqual(a.Get(), 24)

            # Create a new prim that will ultimately become an instance of
            # "foo".
            fd = stage.DefinePrim("/fooDerived", "Scope")
            self.assertFalse(fd.GetAttribute("bar").IsDefined())
            self.assertFalse(fd.GetAttribute("baz").IsDefined())

            a = fd.CreateAttribute("baz", Sdf.ValueTypeNames.Int)
            a.Set(42)
            self.assertEqual(a.Get(), 42)

            # Author the inherits statement to make "fooDerived" an instance of
            # "foo"
            self.assertTrue(fd.GetInherits().AddInherit("/foo"))

            # Verify that opinions from the class come through, but are overridden
            # by any opinions on the instance.
            self.assertTrue(fd.GetAttribute("bar").IsDefined())
            self.assertEqual(fd.GetAttribute("bar").Get(), 42)

            self.assertTrue(fd.GetAttribute("baz").IsDefined())
            self.assertEqual(fd.GetAttribute("baz").Get(), 42)

            # Verify that CreateClassPrim can create a class prim at a subroot
            # path 
            self.assertTrue(stage.GetPrimAtPath("/foo"))
            c = stage.CreateClassPrim("/foo/child")
            self.assertEqual(c.GetPath(), "/foo/child")
            self.assertEqual(c.GetSpecifier(), Sdf.SpecifierClass)

            # Verify that CreateClassPrim can create a class prim at a subroot
            # path even if the parent doesn't exist yet.
            self.assertFalse(stage.GetPrimAtPath("/foo2"))
            c2 = stage.CreateClassPrim("/foo2/child")
            self.assertEqual(c2.GetPath(), "/foo2/child")
            self.assertEqual(c2.GetSpecifier(), Sdf.SpecifierClass)
            self.assertTrue(stage.GetPrimAtPath("/foo2"))

if __name__ == "__main__":
    unittest.main()
