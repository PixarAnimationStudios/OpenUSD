#!/pxrpythonsubst
#
# Copyright 2016 Pixar
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

import os
from pxr import Ar

import unittest
import shutil

class TestArDefaultResolver(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        # Force Ar to use the default resolver implementation.
        Ar.SetPreferredResolver('ArDefaultResolver')

    def test_AnchorRelativePath(self):
        r = Ar.GetResolver()

        self.assertEqual('', r.AnchorRelativePath('', ''))
        self.assertEqual('RelPath', r.AnchorRelativePath('', 'RelPath'))
        self.assertEqual('', r.AnchorRelativePath('RelAnchor', ''))
        self.assertEqual('RelPath',
            r.AnchorRelativePath('RelAnchor', 'RelPath'))
        self.assertEqual('/AbsolutePath',
            r.AnchorRelativePath('/AbsoluteAnchor', '/AbsolutePath'))
        self.assertEqual('/AbsoluteAnchor/Subdir/FileRel.txt',
            r.AnchorRelativePath('/AbsoluteAnchor/ParentFile.txt',
                './Subdir/FileRel.txt'))
        self.assertEqual('/AbsoluteAnchor/Subdir/FileRel.txt',
            r.AnchorRelativePath('/AbsoluteAnchor/ParentDir/ParentFile.txt',
                '../Subdir/FileRel.txt'))

    def test_Resolve(self):
        testFileName = 'test_Resolve.txt'
        testFilePath = os.path.abspath(testFileName)
        with open(testFilePath, 'w') as ofp:
            print >>ofp, 'Garbage'
        
        resolvedPath = Ar.GetResolver().Resolve(testFileName)

        # The resolved path should be absolute.
        self.assertTrue(os.path.isabs(resolvedPath))

        # The resolved path should match the path we constructed.  The
        # os.path.abspath() would appear to be unnecessary because we
        # just checked that it's absolute but it also canonicalizes
        # the path which, on Windows, means the forward slashes
        # returned by the resolver become backslashes.
        self.assertEqual(testFilePath, os.path.abspath(resolvedPath))

if __name__ == '__main__':
    unittest.main()

