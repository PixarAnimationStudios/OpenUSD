#!/pxrpythonsubst
#
# Copyright 2018 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
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
