#!/pxrpythonsubst
#
# Copyright 2020 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
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
