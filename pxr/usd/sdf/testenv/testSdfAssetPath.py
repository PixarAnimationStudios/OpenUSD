#!/pxrpythonsubst
#
# Copyright 2023 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

from __future__ import print_function

from pxr import Sdf
import unittest

class TestSdfAssetPath(unittest.TestCase):
    """Test miscellaneous Sdf.AssetPath behavior"""
    def test_Repr(self):
        for p in [
            Sdf.AssetPath("a"),
            Sdf.AssetPath(authoredPath="a"),
            Sdf.AssetPath("a", "r"),
            Sdf.AssetPath("a", resolvedPath="r"),
            Sdf.AssetPath(authoredPath="a", resolvedPath="r"),
            Sdf.AssetPath(authoredPath="a", evaluatedPath="e",
                          resolvedPath="r")
        ]:
            self.assertEqual(
                p, eval(repr(p)),
                f"{p} did not match result of eval({repr(p)})")

    def test_PositionalArgs(self):
        """Constructing Sdf.AssetPath with > 2 arguments requires keyword
        arguments"""
        with self.assertRaises(TypeError):
            Sdf.AssetPath("authored", "evaluated", resolvedPath="resolved")

        with self.assertRaises(TypeError):
            Sdf.AssetPath("authored",
                          evaluatedPath="evaluated", resolvedPath="resolved")

class TestSdfAssetPathEmpty(unittest.TestCase):
    """Test paths whose path and resolvedPath properties are empty"""
    def setUp(self):
        self.emptyPath = Sdf.AssetPath()
        self.nonEmptyPath = Sdf.AssetPath("/a")
        self.assertFalse(self.emptyPath.path)
        self.assertFalse(self.emptyPath.authoredPath)
        self.assertFalse(self.emptyPath.evaluatedPath)
        self.assertFalse(self.emptyPath.resolvedPath)
        self.assertTrue(self.nonEmptyPath.path)
        self.assertTrue(self.nonEmptyPath.authoredPath)

    def test_EqualityOperators(self):
        self.assertEqual(self.emptyPath, self.emptyPath)
        self.assertNotEqual(self.emptyPath, self.nonEmptyPath)

    def test_ComparisonOperators(self):
        """The empty path should always be less than a non-empty path"""
        self.assertLess(self.emptyPath, self.nonEmptyPath)
        self.assertLessEqual(self.emptyPath, self.nonEmptyPath)
        self.assertLessEqual(self.emptyPath, self.emptyPath)
        self.assertGreater(self.nonEmptyPath, self.emptyPath)
        self.assertGreaterEqual(self.nonEmptyPath, self.emptyPath)
        self.assertGreaterEqual(self.emptyPath, self.emptyPath)

class TestSdfAssetPathUnresolved(unittest.TestCase):
    """Test paths whose resolvedPath property is empty"""
    def setUp(self):
        self.unresolved = Sdf.AssetPath("/unresolved/path")
        self.unresolvedChild = Sdf.AssetPath("/unresolved/path/child")
        self.unresolvedParent = Sdf.AssetPath("/unresolved")
        self.assertEqual(self.unresolved.authoredPath, self.unresolved.path)
        self.assertFalse(self.unresolved.resolvedPath)
        self.assertFalse(self.unresolved.evaluatedPath)

    def test_EqualityOperators(self):
        self.assertEqual(self.unresolved, self.unresolved)
        self.assertEqual(self.unresolved, Sdf.AssetPath(self.unresolved))
        self.assertEqual(self.unresolved, Sdf.AssetPath(self.unresolved.path))
        self.assertEqual(self.unresolved, 
                         Sdf.AssetPath(authoredPath=self.unresolved.path))
        self.assertNotEqual(self.unresolved, self.unresolvedChild)
        self.assertNotEqual(self.unresolved, self.unresolvedParent)

    def test_ComparisonOperators(self):
        """Asset paths should be ordered such that they are less than their
        descendants and greater than their ancestors"""
        self.assertLess(self.unresolved, self.unresolvedChild)
        self.assertLessEqual(self.unresolved, self.unresolved)
        self.assertLessEqual(self.unresolved, self.unresolvedChild)
        self.assertGreater(self.unresolved, self.unresolvedParent)
        self.assertGreaterEqual(self.unresolved, self.unresolved)
        self.assertGreaterEqual(self.unresolved, self.unresolvedParent)

class TestSdfAssetPathResolved(unittest.TestCase):
    """Test paths with valued path and resolvedPaths"""
    def setUp(self):
        self.resolved = Sdf.AssetPath("/unresolved/path", "/resolved/path")
        # `resolvedAlt` shares the unresolved path with `resolved` but
        # has a different resolved result
        self.resolvedAlt = Sdf.AssetPath(self.resolved.path,
                                         f"{self.resolved.resolvedPath}/alt")
        self.unresolved = Sdf.AssetPath(self.resolved.path)
        self.assertFalse(self.unresolved.resolvedPath)
        self.assertEqual(self.resolved.path, self.resolvedAlt.path)
        self.assertEqual(self.resolved.authoredPath,
                         self.unresolved.authoredPath)
        self.assertFalse(self.unresolved.evaluatedPath)
        self.assertNotEqual(self.resolved.resolvedPath,
                            self.resolvedAlt.resolvedPath)

    def test_EqualityOperators(self):
        self.assertEqual(self.resolved, self.resolved)
        self.assertEqual(self.resolved, Sdf.AssetPath(self.resolved))
        self.assertEqual(self.resolved,
                         Sdf.AssetPath(self.resolved.path,
                                       self.resolved.resolvedPath))
        self.assertEqual(self.resolved, 
                         Sdf.AssetPath(self.resolved.path,
                                       resolvedPath=self.resolved.resolvedPath))
        self.assertEqual(self.resolved, 
                         Sdf.AssetPath(authoredPath=self.resolved.path,
                                       resolvedPath=self.resolved.resolvedPath))

        self.assertNotEqual(self.resolved, self.unresolved)
        self.assertNotEqual(self.resolved, self.resolvedAlt)

    def test_ComparisonOperators(self):
        """A resolved path should always be greater than its unresolved
        equivalent"""
        self.assertGreater(self.resolved, self.unresolved)
        self.assertGreaterEqual(self.resolved, self.resolved)
        self.assertGreaterEqual(self.resolved, self.unresolved)

class TestSdfAssetPathEvaluated(unittest.TestCase):
    """Tests paths with evaluated paths"""
    def setUp(self):
        self.pathOnly = Sdf.AssetPath('`"/${VAR}/path"`')
        self.expression = Sdf.AssetPath(authoredPath = '`"/${VAR}/path"`', 
                                        evaluatedPath = "/evaluated/path")
        self.expressionAlt = Sdf.AssetPath(authoredPath = '`"/${VAR}/pathAlt"`', 
                                           evaluatedPath = "/evaluated/pathAlt")
        self.resolved = Sdf.AssetPath(authoredPath = '`"/${VAR}/path"`',
                                      evaluatedPath = "/evaluated/path",
                                      resolvedPath = "/resolved/path")
        self.resolvedAlt = Sdf.AssetPath(authoredPath = '`"/${VAR}/pathAlt"`',
                                         evaluatedPath = "/evaluated/pathAlt",
                                         resolvedPath = "/resolved/pathAlt")

        self.assertFalse(self.expression.resolvedPath)
        self.assertEqual(self.expression.evaluatedPath, "/evaluated/path")
        self.assertEqual(self.expression.path, 
                         self.expression.evaluatedPath)
        self.assertEqual(self.resolved.resolvedPath, "/resolved/path")

    def test_EqualityOperators(self):
        self.assertEqual(self.expression, self.expression)
        self.assertNotEqual(self.expression, self.resolved)

    def test_ComparisonOperatios(self):
        self.assertLess(self.expression, self.resolved)
        self.assertLess(self.expression, self.expressionAlt)
        self.assertLess(self.resolved, self.resolvedAlt)
        self.assertGreater(self.expression, self.pathOnly)

if __name__ == "__main__":
    unittest.main()
