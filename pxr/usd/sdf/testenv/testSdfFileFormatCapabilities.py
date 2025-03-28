#!/pxrpythonsubst
#
# Copyright 2023 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

from __future__ import print_function
import os, tempfile, unittest
from pxr import Plug, Sdf, Tf

class TestSdfCapabilities(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        # Register dso plugins.
        testRoot = os.path.join(os.path.dirname(__file__), 'SdfPlugins')
        testPluginsDso = testRoot + '/lib'
        testPluginsDsoSearch = testPluginsDso + '/*/Resources/'
        Plug.Registry().RegisterPlugins(testPluginsDsoSearch)

    # sanity check for existing file formats which specify nothing in the
    # plugInfo.json file.
    def test_StaticDefault(self):
        ext = '.sdf'
        self.assertTrue(Sdf.FileFormat.FormatSupportsReading(ext))
        self.assertTrue(Sdf.FileFormat.FormatSupportsWriting(ext))
        self.assertTrue(Sdf.FileFormat.FormatSupportsEditing(ext))

    def test_StaticUnwritable(self):
        ext = '.unwritable'
        target = 'sdf'
        self.assertTrue(Sdf.FileFormat.FormatSupportsReading(ext))
        self.assertFalse(Sdf.FileFormat.FormatSupportsWriting(ext))
        self.assertTrue(Sdf.FileFormat.FormatSupportsEditing(ext))

        self.assertTrue(Sdf.FileFormat.FormatSupportsReading(ext, target))
        self.assertFalse(Sdf.FileFormat.FormatSupportsWriting(ext, target))
        self.assertTrue(Sdf.FileFormat.FormatSupportsEditing(ext, target))

    def test_StaticUnreadable(self):
        ext = '.unreadable'
        target = 'test'
        self.assertFalse(Sdf.FileFormat.FormatSupportsReading(ext))
        self.assertTrue(Sdf.FileFormat.FormatSupportsWriting(ext))
        self.assertTrue(Sdf.FileFormat.FormatSupportsEditing(ext))

        self.assertFalse(Sdf.FileFormat.FormatSupportsReading(ext, target))
        self.assertTrue(Sdf.FileFormat.FormatSupportsWriting(ext, target))
        self.assertTrue(Sdf.FileFormat.FormatSupportsEditing(ext, target))

    def test_StaticInvalid(self):
        ext = '.bogus'
        self.assertFalse(Sdf.FileFormat.FormatSupportsReading(ext))
        self.assertFalse(Sdf.FileFormat.FormatSupportsWriting(ext))
        self.assertFalse(Sdf.FileFormat.FormatSupportsEditing(ext))

        ext = '.unreadable'
        target = 'invalid'
        self.assertFalse(Sdf.FileFormat.FormatSupportsReading(ext, target))
        self.assertFalse(Sdf.FileFormat.FormatSupportsWriting(ext, target))
        self.assertFalse(Sdf.FileFormat.FormatSupportsEditing(ext, target))

    def test_InstanceDefault(self):
        sdfFileFormat = Sdf.FileFormat.FindByExtension('.sdf')
        self.assertIsNotNone(sdfFileFormat)
        self.assertTrue(sdfFileFormat.SupportsReading())
        self.assertTrue(sdfFileFormat.SupportsWriting())
        self.assertTrue(sdfFileFormat.SupportsEditing())

    def test_InstanceUnwritable(self):
        unwritableFileFormat = Sdf.FileFormat.FindByExtension('.unwritable')
        self.assertIsNotNone(unwritableFileFormat)
        self.assertTrue(unwritableFileFormat.SupportsReading())
        self.assertFalse(unwritableFileFormat.SupportsWriting())
        self.assertTrue(unwritableFileFormat.SupportsEditing())

    def test_InstanceUnreadable(self):
        unreadableFileFormat = Sdf.FileFormat.FindByExtension('.unreadable')
        self.assertIsNotNone(unreadableFileFormat)
        self.assertFalse(unreadableFileFormat.SupportsReading())
        self.assertTrue(unreadableFileFormat.SupportsWriting())
        self.assertTrue(unreadableFileFormat.SupportsEditing())

    def test_InstanceNonEditable(self):
        uneditableFileFormat = Sdf.FileFormat.FindByExtension('.uneditable')
        self.assertIsNotNone(uneditableFileFormat)

        self.assertTrue(uneditableFileFormat.SupportsReading())
        self.assertTrue(uneditableFileFormat.SupportsWriting())
        self.assertFalse(uneditableFileFormat.SupportsEditing())

    def test_export(self):
        layer = Sdf.Layer.CreateAnonymous()
        with self.assertRaises(Tf.ErrorException):
            layer.Export('file.unwritable')

    def test_import(self):
        # use of tempfile here ensures valid filesystem path
        with tempfile.NamedTemporaryFile(suffix='.writeonly') as temp:
            layer = Sdf.Layer.CreateAnonymous()

            with self.assertRaises(Tf.ErrorException):
                    layer.Import(temp.name)


if __name__ == "__main__":
    unittest.main()
