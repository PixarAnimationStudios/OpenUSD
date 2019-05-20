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

from pxr import Sdf, Tf
import unittest

class TestSdfFileFormat(unittest.TestCase):
    def test_StaticMethods(self):
        print 'Testing static methods on SdfFileFormat...'

        # FindById
        # Note that the id and extension are the same in our case
        sdfFileFormat = Sdf.FileFormat.FindById('sdf')
        self.assertTrue(sdfFileFormat)
        self.assertEqual(sdfFileFormat.GetFileExtensions(), ['sdf'])

        # FindByExtension
        sdfFileFormat = Sdf.FileFormat.FindByExtension('sdf')
        self.assertTrue(sdfFileFormat)
        self.assertEqual(sdfFileFormat.GetFileExtensions(), ['sdf'])

        # GetFileExtension
        self.assertEqual(Sdf.FileFormat.GetFileExtension('foo.sdf'), 'sdf')
        self.assertEqual(Sdf.FileFormat.GetFileExtension('/something/bar/foo.sdf'), 'sdf')
        self.assertEqual(Sdf.FileFormat.GetFileExtension('./bar/baz/foo.sdf'), 'sdf')
         
        # FindAllFileFormatExtensions
        exts = Sdf.FileFormat.FindAllFileFormatExtensions()
        self.assertTrue('sdf' in exts)

if __name__ == "__main__":
    unittest.main()
