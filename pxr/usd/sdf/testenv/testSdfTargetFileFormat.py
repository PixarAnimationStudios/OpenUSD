#!/pxrpythonsubst
#
# Copyright 2025 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

from pxr import Plug, Sdf
import os
import unittest

class TestSdfTargetFileFormat(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        # Register dso plugins.
        testRoot = os.path.join(os.path.dirname(__file__), 'SdfPlugins')
        testPluginsDso = testRoot + '/lib'
        testPluginsDsoSearch = testPluginsDso + '/*/Resources/'
        Plug.Registry().RegisterPlugins(testPluginsDsoSearch)

    def test_CanReadBigCookies(self):
        ext = "test_cookie_format"
        fileFormat = Sdf.FileFormat.FindByExtension(ext)
        self.assertTrue(fileFormat is not None)
        self.assertTrue(
            fileFormat.CanRead("goodCookie.usda"))
        self.assertFalse(
            fileFormat.CanRead("badCookie.usda"))

        # Sanity check: can read any cookie
        ext = "usda"
        fileFormat = Sdf.FileFormat.FindByExtension(ext)
        self.assertTrue(
            fileFormat.CanRead("anyCookie.usda")
        )

if __name__ == "__main__":
    unittest.main()
