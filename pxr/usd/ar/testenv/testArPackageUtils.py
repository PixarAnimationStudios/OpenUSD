#!/pxrpythonsubst
#
# Copyright 2018 Pixar
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

from pxr import Ar, Tf

class TestArPackageUtils(unittest.TestCase):
    def test_IsPackageRelativePath(self):
        self.assertFalse(Ar.IsPackageRelativePath(""))
        self.assertFalse(Ar.IsPackageRelativePath("/tmp/foo.pack"))
        self.assertFalse(Ar.IsPackageRelativePath("foo[0].pack"))

        self.assertTrue(Ar.IsPackageRelativePath("foo.pack[bar.file]"))
        self.assertTrue(
            Ar.IsPackageRelativePath("foo.pack[bar.pack[baz.file]]"))

    def test_JoinPackageRelativePath(self):
        """Test Ar.JoinPackageRelativePath"""
        self.assertEqual(Ar.JoinPackageRelativePath([""]), "")
        self.assertEqual(
            Ar.JoinPackageRelativePath(["foo.pack"]), "foo.pack")
        self.assertEqual(
            Ar.JoinPackageRelativePath(["foo.pack", ""]), "foo.pack")
        self.assertEqual(
            Ar.JoinPackageRelativePath(["foo.pack", "bar.file"]), 
            "foo.pack[bar.file]")
        self.assertEqual(
            Ar.JoinPackageRelativePath(
                ["foo.pack", "bar.pack", "baz.file"]), 
            "foo.pack[bar.pack[baz.file]]")
        self.assertEqual(
            Ar.JoinPackageRelativePath(
                ["foo.pack[bar.pack]", "baz.file"]), 
            "foo.pack[bar.pack[baz.file]]")
        self.assertEqual(
            Ar.JoinPackageRelativePath(["foo[0].pack", "baz.file"]), 
            "foo[0].pack[baz.file]")

        # Corner case: ensure delimiter characters in paths are escaped
        # when enclosed in delimiters by Ar.JoinPackageRelativePath
        self.assertEqual(
            Ar.JoinPackageRelativePath(
                ["foo]a.pack", "bar[b.pack", "baz]c.file"]),
            "foo]a.pack[bar\\[b.pack[baz\\]c.file]]")

    def test_SplitPackageRelativePathInner(self):
        """Test Ar.SplitPackageRelativePathInner"""
        self.assertEqual(Ar.SplitPackageRelativePathInner(""), ("", ""))
        self.assertEqual(
            Ar.SplitPackageRelativePathInner("foo.file"), ("foo.file", ""))
        self.assertEqual(
            Ar.SplitPackageRelativePathInner("foo.pack[bar.file]"), 
            ("foo.pack", "bar.file"))
        self.assertEqual(
            Ar.SplitPackageRelativePathInner("foo.pack[bar.pack[baz.file]]"), 
            ("foo.pack[bar.pack]", "baz.file"))
        self.assertEqual(
            Ar.SplitPackageRelativePathInner("foo[0].pack[bar.file]"), 
            ("foo[0].pack", "bar.file"))

        # Corner case: ensure delimiter characters in paths are unescaped
        # when removed from delimiters by Ar.SplitPackageRelativePathInner.
        self.assertEqual(
            Ar.SplitPackageRelativePathInner(
                "foo]a.pack[bar\\[b.pack[baz\\]c.file]]"),
            ("foo]a.pack[bar\\[b.pack]", "baz]c.file"))

    def test_SplitPackageRelativePathOuter(self):
        """Test Ar.SplitPackageRelativePathOuter"""
        self.assertEqual(Ar.SplitPackageRelativePathOuter(""), ("", ""))
        self.assertEqual(
            Ar.SplitPackageRelativePathOuter("foo.file"), ("foo.file", ""))
        self.assertEqual(
            Ar.SplitPackageRelativePathOuter("foo.pack[bar.file]"), 
            ("foo.pack", "bar.file"))
        self.assertEqual(
            Ar.SplitPackageRelativePathOuter("foo.pack[bar.pack[baz.file]]"), 
            ("foo.pack", "bar.pack[baz.file]"))
        self.assertEqual(
            Ar.SplitPackageRelativePathOuter("foo[0].pack[bar.file]"), 
            ("foo[0].pack", "bar.file"))

        # Corner case: ensure delimiter characters in paths are unescaped
        # when removed from delimiters by Ar.SplitPackageRelativePathOuter.
        self.assertEqual(
            Ar.SplitPackageRelativePathOuter(
                "foo]a.pack[bar\\[b.pack[baz\\]c.file]]"),
            ("foo]a.pack", "bar[b.pack[baz\\]c.file]"))

if __name__ == '__main__':
    unittest.main()
