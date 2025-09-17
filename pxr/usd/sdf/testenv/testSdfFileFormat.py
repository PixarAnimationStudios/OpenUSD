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
        sdfFileFormat = Sdf.FileFormat.FindById('usda')
        self.assertTrue(sdfFileFormat)
        self.assertEqual(sdfFileFormat.GetFileExtensions(), ['usda'])

        # FindByExtension
        sdfFileFormat = Sdf.FileFormat.FindByExtension('usda')
        self.assertTrue(sdfFileFormat)
        self.assertEqual(sdfFileFormat.GetFileExtensions(), ['usda'])
        sdfFileFormatWithArgs = Sdf.FileFormat.FindByExtension(
            'foo.usda', {'target': 'usd', 'documentation': 'doc string'})
        self.assertTrue(sdfFileFormatWithArgs)
        self.assertEqual(sdfFileFormatWithArgs.GetFileExtensions(), ['usda'])

        self.assertEqual(Sdf.FileFormat.FindByExtension('USDA'), sdfFileFormat)
        self.assertEqual(Sdf.FileFormat.FindByExtension('Usda'), sdfFileFormat)
        self.assertEqual(Sdf.FileFormat.FindByExtension('uSDA'), sdfFileFormat)

        # GetFileExtension
        self.assertEqual(Sdf.FileFormat.GetFileExtension('foo.usda'), 'usda')
        self.assertEqual(Sdf.FileFormat.GetFileExtension('/something/bar/foo.usda'), 'usda')
        self.assertEqual(Sdf.FileFormat.GetFileExtension('./bar/baz/foo.usda'), 'usda')
        fileWithArgs = Sdf.Layer.CreateIdentifier(
            'foo.usda', {'documentation' : 'doc string'})
        self.assertEqual(Sdf.FileFormat.GetFileExtension(fileWithArgs), 'usda')
         
        # FindAllFileFormatExtensions
        exts = Sdf.FileFormat.FindAllFileFormatExtensions()
        self.assertTrue('usda' in exts)

        # FindAllDerivedFileFormatExtensions
        exts = Sdf.FileFormat.FindAllDerivedFileFormatExtensions(
            Tf.Type.FindByName('SdfUsdaFileFormat'))
        self.assertTrue('usda' in exts)
        with self.assertRaises(Tf.ErrorException):
            Sdf.FileFormat.FindAllDerivedFileFormatExtensions(Tf.Type())

if __name__ == "__main__":
    unittest.main()
