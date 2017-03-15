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
# Force Ar to fall back to the default resolver implementation.
os.environ['PXR_AR_DISABLE_PLUGIN_RESOLVER'] = '1'
from pxr import Ar

import unittest
import shutil

class TestArDefaultResolver(unittest.TestCase):
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
        
        r = Ar.GetResolver()
        self.assertEqual(testFilePath, r.Resolve(testFileName))

if __name__ == '__main__':
    unittest.main()

