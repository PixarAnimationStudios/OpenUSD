#!/pxrpythonsubst
#
# Copyright 2025 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
from pathlib import Path
import os
import unittest

# Force the use of ArDefaultResolver as the primary resolver for
# this test.
os.environ["PXR_AR_DISABLE_PLUGIN_RESOLVER"] = "1"

from pxr import Plug, Ar, Tf

class TestArPackageResolver(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        # Register test resolver plugins
        # Test plugins are installed relative to this script
        testRoot = os.path.join(
            os.path.dirname(os.path.abspath(__file__)), 'ArPlugins')

        pr = Plug.Registry()

        testPackageResolverPath = os.path.join(
            testRoot, 'lib/TestArPackageResolver*/Resources/')
        pr.RegisterPlugins(testPackageResolverPath)

    def assertPathsEqual(self, path1, path2):
        # Flip backslashes to forward slashes to accommodate platform
        # differences. 
        self.assertEqual(
            os.path.normpath(str(path1)), 
            os.path.normpath(str(path2)))

    def test_Setup(self):
        # Verify that our test plugin was registered properly.
        pr = Plug.Registry()
        self.assertTrue(pr.GetPluginWithName('TestArPackageResolver'))
        self.assertTrue(Tf.Type.FindByName('_TestPackageResolver'))

    def test_Resolver(self):
        def _test(packageFileName):
            # Create an empty test file for testing.
            Path(packageFileName).touch()

            # Create a test filename like 'foo.package[packaged_file]'.
            # The packaged filename doesn't matter -- our test package
            # resolver will just return it as-is.
            testPackagedFilePath = Ar.JoinPackageRelativePath(
                packageFileName, 'packaged_file')

            resolver = Ar.GetResolver()
            self.assertPathsEqual(
                resolver.Resolve(testPackagedFilePath),
                os.path.abspath(testPackagedFilePath))

        _test("test.package")

        # Verify that Ar is case-insensitive when checking for a
        # package resolver.
        _test("test.PACKAGE")

if __name__ == '__main__':
    unittest.main()
