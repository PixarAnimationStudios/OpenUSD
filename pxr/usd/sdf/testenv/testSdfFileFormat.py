#!/pxrpythonsubst
#
# Copyright 2018 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

from __future__ import print_function

from pxr import Sdf, Tf
import unittest

class TestSdfFileFormat(unittest.TestCase):
    def test_StaticMethods(self):
        print('Testing static methods on SdfFileFormat...')

        # FindById
        # Note that the id and extension are the same in our case
        sdfFileFormat = Sdf.FileFormat.FindById('sdf')
        self.assertTrue(sdfFileFormat)
        self.assertEqual(sdfFileFormat.GetFileExtensions(), ['sdf'])

        # FindByExtension
        sdfFileFormat = Sdf.FileFormat.FindByExtension('sdf')
        self.assertTrue(sdfFileFormat)
        self.assertEqual(sdfFileFormat.GetFileExtensions(), ['sdf'])
        sdfFileFormatWithArgs = Sdf.FileFormat.FindByExtension(
            'foo.sdf', {'target': 'sdf', 'documentation': 'doc string'})
        self.assertTrue(sdfFileFormatWithArgs)
        self.assertEqual(sdfFileFormatWithArgs.GetFileExtensions(), ['sdf'])

        self.assertEqual(Sdf.FileFormat.FindByExtension('SDF'), sdfFileFormat)
        self.assertEqual(Sdf.FileFormat.FindByExtension('Sdf'), sdfFileFormat)
        self.assertEqual(Sdf.FileFormat.FindByExtension('sDF'), sdfFileFormat)

        # GetFileExtension
        self.assertEqual(Sdf.FileFormat.GetFileExtension('foo.sdf'), 'sdf')
        self.assertEqual(Sdf.FileFormat.GetFileExtension('/something/bar/foo.sdf'), 'sdf')
        self.assertEqual(Sdf.FileFormat.GetFileExtension('./bar/baz/foo.sdf'), 'sdf')
        fileWithArgs = Sdf.Layer.CreateIdentifier(
            'foo.sdf', {'documentation' : 'doc string'})
        self.assertEqual(Sdf.FileFormat.GetFileExtension(fileWithArgs), 'sdf')
         
        # FindAllFileFormatExtensions
        exts = Sdf.FileFormat.FindAllFileFormatExtensions()
        self.assertTrue('sdf' in exts)

        # FindAllDerivedFileFormatExtensions
        exts = Sdf.FileFormat.FindAllDerivedFileFormatExtensions(
            Tf.Type.FindByName('SdfTextFileFormat'))
        self.assertTrue('sdf' in exts)
        with self.assertRaises(Tf.ErrorException):
            Sdf.FileFormat.FindAllDerivedFileFormatExtensions(Tf.Type())

if __name__ == "__main__":
    unittest.main()
