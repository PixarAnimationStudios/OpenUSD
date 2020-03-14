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
