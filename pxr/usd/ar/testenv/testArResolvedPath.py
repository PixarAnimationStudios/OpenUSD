#!/pxrpythonsubst
#
# Copyright 2020 Pixar
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
#
import unittest

from pxr import Ar

class TestArResolvedPath(unittest.TestCase):
    def test_Basic(self):
        self.assertFalse(bool(Ar.ResolvedPath()))
        self.assertEqual(Ar.ResolvedPath(), Ar.ResolvedPath(""))
        self.assertEqual(Ar.ResolvedPath(), "")

        self.assertTrue(bool(Ar.ResolvedPath("/foo")))
        self.assertEqual(Ar.ResolvedPath("/foo"), Ar.ResolvedPath("/foo"))
        self.assertNotEqual(Ar.ResolvedPath("/foo"), Ar.ResolvedPath("/bar"))
        
        self.assertTrue(Ar.ResolvedPath("/foo") > Ar.ResolvedPath("/bar"))
        self.assertTrue(Ar.ResolvedPath("/foo") >= Ar.ResolvedPath("/bar"))
        self.assertFalse(Ar.ResolvedPath("/foo") < Ar.ResolvedPath("/bar"))
        self.assertFalse(Ar.ResolvedPath("/foo") <= Ar.ResolvedPath("/bar"))

        self.assertEqual(Ar.ResolvedPath("/foo"), "/foo")
        self.assertNotEqual(Ar.ResolvedPath("/foo"), "/bar")
        self.assertTrue(Ar.ResolvedPath("/foo") > "/bar")
        self.assertTrue(Ar.ResolvedPath("/foo") >= "/bar")
        self.assertFalse(Ar.ResolvedPath("/foo") < "/bar")
        self.assertFalse(Ar.ResolvedPath("/foo") <= "/bar")

        self.assertEqual("/foo", Ar.ResolvedPath("/foo"))
        self.assertNotEqual("/bar", Ar.ResolvedPath("/foo"))
        self.assertFalse("/bar" > Ar.ResolvedPath("/foo"))
        self.assertFalse("/bar" >= Ar.ResolvedPath("/foo"))
        self.assertTrue("/bar" < Ar.ResolvedPath("/foo"))
        self.assertTrue("/bar" <= Ar.ResolvedPath("/foo"))

    def test_GetPathString(self):
        self.assertEqual(Ar.ResolvedPath().GetPathString(), "")
        self.assertEqual(Ar.ResolvedPath("/foo").GetPathString(), "/foo")

    def test_Repr(self):
        self.assertEqual(repr(Ar.ResolvedPath()), "Ar.ResolvedPath()")
        self.assertEqual(repr(Ar.ResolvedPath("/foo")), 
                         "Ar.ResolvedPath('/foo')")

    def test_Str(self):
        self.assertEqual(str(Ar.ResolvedPath()), "")
        self.assertEqual(str(Ar.ResolvedPath("/foo")), "/foo")

    def test_Hash(self):
        hash(Ar.ResolvedPath())
        hash(Ar.ResolvedPath("/foo"))

if __name__ == '__main__':
    unittest.main()
